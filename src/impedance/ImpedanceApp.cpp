#include "ImpedanceApp.h"
#include <string.h>
#include <math.h>
#include "ad5934_math.h"
namespace {
constexpr uint32_t ACQUISITION_TIMEOUT_MS = 500;
constexpr uint32_t APPLICATION_PHASE_TIMEOUT_MS = 1500;
constexpr uint32_t STATUS_POLL_INTERVAL_MS = 5;
constexpr uint8_t CONFIG_FIELD_COUNT = 11;
}
ImpedanceApp::ImpedanceApp(AD5934& driver, MuxApp& mux) : driver_(driver), mux_(mux) {}
void ImpedanceApp::update() { if (isBusy()) advanceMeasurement(); refreshPublicStatus(); }
bool ImpedanceApp::isBusy() const {
  return measurementState_ != MeasurementState::Idle && measurementState_ != MeasurementState::Complete &&
      measurementState_ != MeasurementState::Error && measurementState_ != MeasurementState::Cancelled;
}
void ImpedanceApp::recordError(ImpedanceDiagnostics::ErrorCode code,
                               ImpedanceDiagnostics::Subsystem subsystem,
                               const char* detail, bool recoverable) {
  using namespace ImpedanceDiagnostics;
  lastErrorRecord_ = ErrorRecord{code, subsystem,
      static_cast<uint8_t>(measurementState_), millis(), detail, recoverable,
      RecoveryState::NotAttempted};
  publicStatus_.lastError = code;
  publicStatus_.activeError = code;
  if (static_cast<uint8_t>(code) < static_cast<uint8_t>(ErrorCode::Count))
    ++errorCounters_[static_cast<uint8_t>(code)];
}

void ImpedanceApp::refreshPublicStatus() {
  publicStatus_.busy = isBusy();

  publicStatus_.elapsedMs = publicStatus_.busy ? millis() - measurementStartedMs_ :
      measurementElapsedMs_;
  publicStatus_.lastError = lastErrorRecord_.code;
  publicStatus_.hardwareReady = publicStatus_.ad5934Communication &&
      publicStatus_.adg715Communication && publicStatus_.configurationVerified &&
      publicStatus_.operational && publicStatus_.diagnosticComplete &&
      publicStatus_.adg715MaskKnown;
}

ImpedanceApp::ImpedanceStatus ImpedanceApp::statusSnapshot() const {
  ImpedanceStatus snapshot = publicStatus_;
  snapshot.busy = isBusy();

  snapshot.elapsedMs = snapshot.busy ? millis() - measurementStartedMs_ :
      measurementElapsedMs_;
  snapshot.lastError = lastErrorRecord_.code;
  if (mux_.hasObservedMask()) {
    snapshot.adg715Mask = mux_.observedMask();
    snapshot.adg715MaskKnown = true;
  }
  snapshot.hardwareReady = snapshot.ad5934Communication && snapshot.adg715Communication &&
      snapshot.configurationVerified && snapshot.operational && snapshot.diagnosticComplete &&
      snapshot.adg715MaskKnown;
  return snapshot;
}

bool ImpedanceApp::configureSingle(MuxApp::CalibrationPreset preset, uint32_t frequencyHz,
                                   uint16_t settlingCycles) {
  if (isBusy() || settlingCycles > 511) return false;
  uint32_t code = 0;
  MuxApp::CalibrationInfo info{};
  if (!AD5934::frequencyToCode(frequencyHz, profile_.mclkHz, code) ||
      !mux_.resolveCalibrationPreset(preset, info)) return false;
  profile_.calibrationPreset = preset;
  profile_.frequencyHz = frequencyHz;
  profile_.settlingCycles = settlingCycles;
  return true;
}

bool ImpedanceApp::cancelOperation() {
  if (!isBusy()) return false;
  failMeasurement("cancelada por Manager", true);
  return true;
}

bool ImpedanceApp::requestRecovery(bool busReady) {
  if (!busReady || isBusy()) return false;
  recover();
  return statusSnapshot().hardwareReady;
}

void ImpedanceApp::recover() {
  using namespace ImpedanceDiagnostics;
  if (isBusy()) { recordError(ErrorCode::Busy, Subsystem::Application, "recovery requested while busy", false); return; }
  publicStatus_.hardwareReady = false;
  publicStatus_.operational = false;
  publicStatus_.configurationVerified = false;
  publicStatus_.diagnosticComplete = false;
  publicStatus_.adg715MaskKnown = false;

  const bool ad5934Ack = driver_.begin();
  const bool adgAck = mux_.ping();
  publicStatus_.ad5934Communication = ad5934Ack;
  publicStatus_.adg715Communication = adgAck;
  uint8_t mask = 0;
  const bool maskRead = adgAck && mux_.readMask(mask);
  if (maskRead) { publicStatus_.adg715Mask = mask; publicStatus_.adg715MaskKnown = true; }
  uint16_t before = 0, after = 0;
  // enterStandby also accepts an already safe power-down. The instrument's
  // public contract specifically requires standby, including early stop.
  const bool standby = ad5934Ack && driver_.enterStandby(before, after) &&
      driver_.wakeToStandby(before, after);
  uint8_t statusByte = 0;
  const bool statusRead = standby && driver_.readStatus(statusByte);
  uint32_t startCode = 0, incrementCode = 0;
  uint16_t incrementCount = 0, settlingWord = 0;
  const bool configRead = statusRead &&
      driver_.readRegister24(AD5934::START_FREQUENCY_MSB, startCode) &&
      driver_.readRegister24(AD5934::FREQUENCY_INCREMENT_MSB, incrementCode) &&
      driver_.readRegister16(AD5934::NUMBER_INCREMENTS_MSB, incrementCount) &&
      driver_.readRegister16(AD5934::SETTLING_CYCLES_MSB, settlingWord);
  const bool configValid = configRead &&
      driver_.validateWrite(AD5934::START_FREQUENCY_MSB, 3, startCode) &&
      driver_.validateWrite(AD5934::FREQUENCY_INCREMENT_MSB, 3, incrementCode) &&
      driver_.validateWrite(AD5934::NUMBER_INCREMENTS_MSB, 2, incrementCount) &&
      driver_.validateWrite(AD5934::SETTLING_CYCLES_MSB, 2, settlingWord);
  const bool knownPreset = maskRead && mux_.knownPresetMask(mask);
  publicStatus_.configurationVerified = standby && configValid;
  publicStatus_.operational = publicStatus_.configurationVerified && knownPreset;
  publicStatus_.diagnosticComplete = ad5934Ack && adgAck && maskRead && configRead;
  publicStatus_.lastOperationSuccessful = publicStatus_.operational;
  if (publicStatus_.operational) {
    publicStatus_.activeError = ErrorCode::None;
    if (lastErrorRecord_.code != ErrorCode::None) lastErrorRecord_.recovery = RecoveryState::Complete;

  } else {
    if (lastErrorRecord_.code == ErrorCode::None)
      recordError(ErrorCode::RecoveryFailed, Subsystem::Application, "recovery verification incomplete", true);
    else {
      publicStatus_.activeError = ErrorCode::RecoveryFailed;
      ++errorCounters_[static_cast<uint8_t>(ErrorCode::RecoveryFailed)];
    }
    lastErrorRecord_.recovery = (ad5934Ack || adgAck) ? RecoveryState::Partial : RecoveryState::Failed;

  }
  refreshPublicStatus();
}

void ImpedanceApp::failMeasurement(const char* message, bool recover) {
  measurementError_ = message;

  driverError_ = driver_.lastError();
  cancelled_ = strstr(message, "cancel") != nullptr;
  using namespace ImpedanceDiagnostics;
  ErrorCode code = ErrorCode::InvalidState;
  Subsystem subsystem = Subsystem::Application;
  bool recoverable = true;
  if (driverError_.code == AD5934::ErrorCode::AddressNack || driverError_.code == AD5934::ErrorCode::DataNack) {
    code = ErrorCode::I2cNack; subsystem = Subsystem::Ad5934;
  } else if (driverError_.code == AD5934::ErrorCode::Timeout) {
    code = ErrorCode::I2cTimeout; subsystem = Subsystem::Ad5934;
  } else if (driverError_.code == AD5934::ErrorCode::Bus || driverError_.code == AD5934::ErrorCode::ShortRead) {
    code = ErrorCode::I2cError; subsystem = Subsystem::Ad5934;
  } else if (strstr(message, "timeout")) {
    code = ErrorCode::DftTimeout;
  } else if (strstr(message, "readback") || strstr(message, "verificar")) {
    code = ErrorCode::RegisterVerifyFailed; subsystem = Subsystem::Ad5934;
  } else if (strstr(message, "preset") || strstr(message, "ADG715")) {
    code = ErrorCode::Adg715VerifyFailed; subsystem = Subsystem::Adg715;
  }
  if (!cancelled_) recordError(code, subsystem, message, recoverable);
  publicStatus_.lastOperationSuccessful = false;
  if (recover) setMeasurementState(MeasurementState::Stopping);
  else setMeasurementState(MeasurementState::Error);
}

bool ImpedanceApp::startMeasurement(bool busReady, uint8_t routeMask) {
  if (!busReady) { return false; }
  if (isBusy()) { return false; }

  refreshPublicStatus();
  if (!publicStatus_.hardwareReady) {
    recordError(ImpedanceDiagnostics::ErrorCode::InvalidState, ImpedanceDiagnostics::Subsystem::Application,
                "hardware readiness unknown; run diag or recover", true);

    return false;
  }
  if (!AD5934::frequencyToCode(profile_.frequencyHz, profile_.mclkHz, startCode_)) return false;
  incrementCode_ = 0;
  activeStartHz_ = profile_.frequencyHz;
  activeIncrements_ = 0;
  MuxApp::CalibrationInfo selectedCalibration{};
  if (!(routeMask == 0x41 ? mux_.pt100Info(selectedCalibration) :
      routeMask == 0x81 || routeMask == 0x82 ? mux_.ecInfo(routeMask, selectedCalibration) :
      mux_.resolveCalibrationPreset(profile_.calibrationPreset, selectedCalibration))) {
    return false;
  }
  if (!profile_.externalClock || profile_.excitationRange != 1 || profile_.pgaGain != 1 ||
      (profile_.numberOfIncrements != 0 || incrementCode_ != 0)) {

    return false;
  }
  cancelled_ = false;  // A new acquisition must not inherit a previous cancellation.
  measurementStartedMs_ = millis();
  measurementElapsedMs_ = 0;
  if (!driver_.begin()) { failMeasurement("fallo de comunicacion AD5934", true); return false; }
  uint16_t control;
  if (!driver_.readRegister16(AD5934::CONTROL_HIGH, control)) {
    failMeasurement("no se pudo leer el control inicial", true); return false;
  }
  const uint8_t mode = static_cast<uint8_t>((control >> 12) & 0x0F);
  if ((mode != 0x0A && mode != 0x0B) || (control & 0x00FF) != 0x08) {

    failMeasurement("estado inicial AD5934 no seguro", true);
    return false;
  }
  publicStatus_.ad5934Communication = true;
  uint8_t observedMask = 0;
  if (!mux_.readMask(observedMask)) {
    recordError(ImpedanceDiagnostics::ErrorCode::I2cError, ImpedanceDiagnostics::Subsystem::Adg715,
                "ADG715 preflight read failed", true);

    return false;
  }
  publicStatus_.adg715Communication = true;
  publicStatus_.adg715Mask = observedMask;
  publicStatus_.adg715MaskKnown = true;
  publicStatus_.operational = mux_.knownPresetMask(observedMask);
  if (!publicStatus_.operational) {
    recordError(ImpedanceDiagnostics::ErrorCode::InvalidState, ImpedanceDiagnostics::Subsystem::Adg715,
                "ADG715 mask is not a recognized route", true);

    return false;
  }
  publicStatus_.diagnosticComplete = true;
  if (!(routeMask == 0x41 ? mux_.applyPt100() :
      routeMask == 0x81 || routeMask == 0x82 ? mux_.applyEc(routeMask) :
      mux_.applyCalibrationPreset(profile_.calibrationPreset))) {

    failMeasurement("no se pudo aplicar/verificar preset de calibracion", true);
    return false;
  }
  publicStatus_.adg715Mask = selectedCalibration.mask;
  publicStatus_.adg715MaskKnown = true;
  publicStatus_.adg715Communication = true;
  publicStatus_.operational = true;
  measurementCalibration_ = selectedCalibration;
  hasResult_ = false;
  measurementError_ = nullptr;
  configPhase_ = 0;
  verifyPhase_ = 0;
  setMeasurementState(MeasurementState::Configuring);
  publicStatus_.lastOperationSuccessful = false;
  return true;
}

void ImpedanceApp::setMeasurementState(MeasurementState state) {
  measurementState_ = state;
  stateStartedMs_ = millis();
}

void ImpedanceApp::finishStopping() {
  uint16_t previous = 0, verified = 0;
  if (!driver_.enterStandby(previous, verified) || !driver_.wakeToStandby(previous, verified)) {
    driverError_ = driver_.lastError();
    measurementError_ = "fallo de recuperacion a standby";

    setMeasurementState(MeasurementState::Error);
    publicStatus_.operational = false;
    publicStatus_.configurationVerified = false;
    publicStatus_.diagnosticComplete = false;
    recordError(ImpedanceDiagnostics::ErrorCode::RecoveryFailed,
        ImpedanceDiagnostics::Subsystem::Ad5934, measurementError_, true);
    publicStatus_.hardwareReady = false;
    if (lastErrorRecord_.code != ImpedanceDiagnostics::ErrorCode::None) lastErrorRecord_.recovery = ImpedanceDiagnostics::RecoveryState::Failed;
    return;
  }
  publicStatus_.operational = cancelled_ || !measurementError_;
  if (measurementError_ && lastErrorRecord_.code != ImpedanceDiagnostics::ErrorCode::None) {
    lastErrorRecord_.recovery = ImpedanceDiagnostics::RecoveryState::Complete;
    publicStatus_.activeError = ImpedanceDiagnostics::ErrorCode::None;
    publicStatus_.configurationVerified = false;
    publicStatus_.diagnosticComplete = false;
  }
  measurementElapsedMs_ = millis() - measurementStartedMs_;

  if (cancelled_) {
    measurementError_ = "cancelada por usuario; AD5934 en standby";
    setMeasurementState(MeasurementState::Cancelled);

  } else if (measurementError_) {
    setMeasurementState(MeasurementState::Error);

  } else {
    result_ = Result{};
    result_.real = pendingReal_;
    result_.imaginary = pendingImaginary_;
    result_.frequencyHz = profile_.frequencyHz;
    result_.effectiveFrequencyHz = AD5934Math::frequencyFromCode(startCode_, profile_.mclkHz);
    result_.elapsedMs = measurementElapsedMs_;
    result_.calibration = measurementCalibration_;
    result_.magnitude = AD5934::magnitude(result_.real, result_.imaginary);
    result_.phaseDegrees = AD5934Math::phaseDegrees(result_.real, result_.imaginary);
    result_.valid = true;
    hasResult_ = true;
    setMeasurementState(MeasurementState::Complete);

    publicStatus_.lastOperationSuccessful = true;
  }
}

void ImpedanceApp::advanceMeasurement() {
  uint8_t byte = 0;
  const uint32_t now = millis();
  const bool phaseTimed = measurementState_ == MeasurementState::Configuring ||
      measurementState_ == MeasurementState::PreparingStandby ||
      measurementState_ == MeasurementState::Initializing;
  if (phaseTimed && now - stateStartedMs_ >= APPLICATION_PHASE_TIMEOUT_MS) {
    failMeasurement("timeout en fase de control/configuracion AD5934", true);
    return;
  }
  switch (measurementState_) {
    case MeasurementState::Configuring: {
      if (configPhase_ == 0) {
        if (!driver_.configureRange1PgaX1()) { failMeasurement("fallo al configurar control Range 1/PGA x1", true); return; }
        configPhase_ = 1;
        setMeasurementState(MeasurementState::PreparingStandby);
        return;
      } else {
        const uint8_t field = configPhase_ - 1;
        if (field < 3) {
          byte = static_cast<uint8_t>(startCode_ >> (16 - 8 * field));
          if (!driver_.writeRegister8(AD5934::START_FREQUENCY_MSB + field, byte)) { failMeasurement("fallo al escribir frecuencia DDS", true); return; }
        } else if (field < 6) {
          byte = static_cast<uint8_t>(incrementCode_ >> (16 - 8 * (field - 3)));
          if (!driver_.writeRegister8(AD5934::FREQUENCY_INCREMENT_MSB + field - 3, byte)) { failMeasurement("fallo al escribir incremento", true); return; }
        } else if (field < 8) {
          byte = static_cast<uint8_t>(activeIncrements_ >> (field == 6 ? 8 : 0));
          if (!driver_.writeRegister8(AD5934::NUMBER_INCREMENTS_MSB + field - 6, byte)) { failMeasurement("fallo al escribir numero de incrementos", true); return; }
        } else {
          const uint16_t settling = profile_.settlingCycles;
          byte = field == 8 ? static_cast<uint8_t>(settling >> 8) : static_cast<uint8_t>(settling);
          if (!driver_.writeRegister8(field == 8 ? AD5934::SETTLING_CYCLES_MSB : AD5934::SETTLING_CYCLES_LSB, byte)) { failMeasurement("fallo al escribir settling cycles", true); return; }
        }
      }
      if (++configPhase_ >= CONFIG_FIELD_COUNT) { verifyPhase_ = 0; setMeasurementState(MeasurementState::Initializing); }
      return;
    }
    case MeasurementState::PreparingStandby: {
      uint16_t previous = 0, verified = 0;
      if (!driver_.wakeToStandby(previous, verified)) {
        failMeasurement("fallo al pasar de power-down a standby", true);
        return;
      }
      setMeasurementState(MeasurementState::Configuring);
      return;
    }
    case MeasurementState::Initializing:
      if (verifyPhase_ < CONFIG_FIELD_COUNT) {
        const uint8_t field = verifyPhase_++;
        uint8_t reg;
        uint8_t expected;
        if (field == 0) { reg = AD5934::CONTROL_HIGH; expected = 0xB1; }
        else if (field < 4) { reg = AD5934::START_FREQUENCY_MSB + field - 1; expected = static_cast<uint8_t>(startCode_ >> (16 - 8 * (field - 1))); }
        else if (field < 7) { reg = AD5934::FREQUENCY_INCREMENT_MSB + field - 4; expected = static_cast<uint8_t>(incrementCode_ >> (16 - 8 * (field - 4))); }
        else if (field < 9) { reg = AD5934::NUMBER_INCREMENTS_MSB + field - 7; expected = static_cast<uint8_t>(activeIncrements_ >> (field == 7 ? 8 : 0)); }
        else { reg = field == 9 ? AD5934::SETTLING_CYCLES_MSB : AD5934::SETTLING_CYCLES_LSB; expected = field == 9 ? static_cast<uint8_t>(profile_.settlingCycles >> 8) : static_cast<uint8_t>(profile_.settlingCycles); }
        if (!driver_.readRegister8(reg, byte)) { failMeasurement("fallo al verificar perfil AD5934", true); return; }
        if (byte != expected) { failMeasurement("readback de perfil AD5934 no coincide", true); return; }
        return;
      }
      if (!driver_.sendCommand(AD5934::ControlCommand::InitializeStartFrequency)) { failMeasurement("fallo al inicializar frecuencia", true); return; }
      setMeasurementState(MeasurementState::Settling);
      return;
    case MeasurementState::Settling: {
      const uint32_t waitMs = (static_cast<uint32_t>(profile_.startSettlingCycles) * 1000U + activeStartHz_ - 1) / activeStartHz_;
      if (now - stateStartedMs_ < (waitMs ? waitMs : 1)) return;
      if (!driver_.sendCommand(AD5934::ControlCommand::StartFrequencySweep)) { failMeasurement("fallo al iniciar DFT", true); return; }
      lastPollMs_ = now;
      acquisitionStartedMs_ = now;
      setMeasurementState(MeasurementState::Acquiring);
      return;
    }
    case MeasurementState::Acquiring:
      if (now - acquisitionStartedMs_ >= ACQUISITION_TIMEOUT_MS) { failMeasurement("timeout esperando D1 de datos DFT", true); return; }
      if (now - lastPollMs_ < STATUS_POLL_INTERVAL_MS) return;
      lastPollMs_ = now;
      if (!driver_.readStatus(byte)) { failMeasurement("fallo I2C leyendo status DFT", true); return; }
      if (byte & AD5934::STATUS_DATA_VALID) setMeasurementState(MeasurementState::ReadingReal);
      return;
    case MeasurementState::ReadingReal:
      if (!driver_.readReal(pendingReal_)) { failMeasurement("fallo leyendo REAL", true); return; }
      setMeasurementState(MeasurementState::ReadingImaginary);
      return;
    case MeasurementState::ReadingImaginary:
      if (!driver_.readImaginary(pendingImaginary_)) { failMeasurement("fallo leyendo IMAG", true); return; }
      result_.imaginary = pendingImaginary_;
      measurementError_ = nullptr;
      setMeasurementState(MeasurementState::Stopping);
      return;
    case MeasurementState::Stopping:
      finishStopping();
      return;
    case MeasurementState::Idle:
    case MeasurementState::Complete:
    case MeasurementState::Error:
    case MeasurementState::Cancelled:
      return;
  }
}
