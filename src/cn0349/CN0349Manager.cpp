#include "CN0349Manager.h"

#include <string.h>
#include <ctype.h>
#include <math.h>
#include "../terminal/Terminal.h"

CN0349Manager::CN0349Manager(ImpedanceApp& impedance, Stream& output)
    : impedance_(impedance), output_(output) {}

bool CN0349Manager::begin(Terminal& terminal, bool busReady) {
  busReady_ = busReady;
  initialized_ = terminal.attachModule(this, &CN0349Manager::dispatch,
      &CN0349Manager::busy, "  cn0349 help - API de alto nivel del subsistema CN-0349");
  state_ = initialized_ ? CN0349State::Idle : CN0349State::NotInitialized;
  return initialized_;
}

void CN0349Manager::update() {
  if (!initialized_) return;
  impedance_.update();
  if (isCalibrationRunning()) advanceCalibration();
  else if (isImpedanceRunning()) advanceImpedance();
  else if (isEcRunning()) advanceEc();
  else synchronizeOutcome();
  advanceTemperature();
  scheduleTemperature();
  if (preparationPending_ && !isBusy()) {
    preparationPending_ = false;
    CN0349Calibration selected{};
    const bool ready = calibrationState_ == CN0349CalibrationState::Complete && isReady() &&
        getCalibration(impedanceCalibration_, selected) && impedanceCalibrationCompatible(selected);
    if (ready) {
      output_.println("Arranque COMPLETE: cuatro calibraciones listas. Sistema preparado para medir.");
      output_.print("Calibracion RTD seleccionada: "); output_.println(presetName(impedanceCalibration_));
      output_.println("Use cn0349 temperature start o cn0349 impedance start.");
    } else {
      output_.print("Arranque FAILED/CANCELLED: sistema no preparado; calibracion error=");
      output_.println(calibrationErrorName(calibrationError_));
      output_.println("Consulte cn0349 cal status y cn0349 status.");
    }
  }
}

bool CN0349Manager::isReady() const { return impedance_.statusSnapshot().hardwareReady; }
bool CN0349Manager::isBusy() const { return temperatureContinuous_ || impedance_.isBusy() || isCalibrationRunning() || isImpedanceRunning() || isEcRunning(); }
bool CN0349Manager::busy(void* context) { return static_cast<CN0349Manager*>(context)->isBusy(); }

MuxApp::CalibrationPreset CN0349Manager::toMuxPreset(CN0349Preset preset) {
  return static_cast<MuxApp::CalibrationPreset>(preset);
}

const char* CN0349Manager::presetName(CN0349Preset preset) {
  switch (preset) {
    case CN0349Preset::CalHigh100R: return "CAL_HIGH_100R";
    case CN0349Preset::CalHigh1K: return "CAL_HIGH_1K";
    case CN0349Preset::CalLow1K: return "CAL_LOW_1K";
    case CN0349Preset::CalLow10K: return "CAL_LOW_10K";
  }
  return "UNKNOWN";
}

bool CN0349Manager::parsePreset(const char* name, CN0349Preset& preset) const {
  for (uint8_t presetIndex = 0; presetIndex < 4; ++presetIndex) {
    const auto candidate = static_cast<CN0349Preset>(presetIndex);
    MuxApp::CalibrationInfo info{};
    if (!impedance_.presetInfo(toMuxPreset(candidate), info)) continue;
    size_t charIndex = 0;
    while (name[charIndex] && info.name[charIndex] &&
           toupper(static_cast<unsigned char>(name[charIndex])) == info.name[charIndex]) ++charIndex;
    if (name[charIndex] == '\0' && info.name[charIndex] == '\0') { preset = candidate; return true; }
  }
  return false;
}

bool CN0349Manager::setPreset(CN0349Preset preset) {
  if (!initialized_ || isBusy()) return false;
  return impedance_.setRequestedPreset(toMuxPreset(preset));
}

CN0349Preset CN0349Manager::getPreset() const {
  return static_cast<CN0349Preset>(impedance_.profile().calibrationPreset);
}

CN0349SingleConfig CN0349Manager::getSingleConfig() const {
  const auto& profile = impedance_.profile();
  return CN0349SingleConfig{getPreset(), profile.frequencyHz, profile.settlingCycles};
}

bool CN0349Manager::configureSingle(const CN0349SingleConfig& config) {
  if (!initialized_ || isBusy()) return false;
  return impedance_.configureSingle(toMuxPreset(config.preset), config.frequencyHz,
                                    config.settlingCycles);
}

bool CN0349Manager::startSingle() {
  if (!initialized_ || isBusy()) return false;
  resultOperationValid_ = false;
  activeOperation_ = CN0349Operation::Single;
  lastOperation_ = activeOperation_;
  lastOutcome_ = CN0349Outcome::None;
  cancelledByManager_ = false;
  if (impedance_.requestSingle(busReady_)) { resultOperationValid_ = true; state_ = CN0349State::RunningSingle; return true; }
  if (impedance_.isBusy()) { state_ = CN0349State::RunningSingle; return false; }
  activeOperation_ = CN0349Operation::None;
  lastOutcome_ = CN0349Outcome::Failed;
  state_ = CN0349State::Error;
  return false;
}

bool CN0349Manager::configureSweep(const CN0349SweepConfig& config) {
  if (!initialized_ || isBusy()) return false;
  return impedance_.configureSweep(toMuxPreset(config.preset), config.startHz,
      config.stepHz, config.increments, config.settlingCycles);
}

CN0349SweepConfig CN0349Manager::getSweepConfig() const {
  const auto& sweep = impedance_.sweepConfiguration();
  return CN0349SweepConfig{getPreset(), sweep.startHz, sweep.incrementHz, sweep.increments,
                           impedance_.profile().settlingCycles};
}

bool CN0349Manager::startSweep() {
  if (!initialized_ || isBusy()) return false;
  resultOperationValid_ = false;
  activeOperation_ = CN0349Operation::Sweep;
  lastOperation_ = activeOperation_;
  lastOutcome_ = CN0349Outcome::None;
  cancelledByManager_ = false;
  if (impedance_.requestSweep(busReady_)) { resultOperationValid_ = true; state_ = CN0349State::RunningSweep; return true; }
  if (impedance_.isBusy()) { state_ = CN0349State::RunningSweep; return false; }
  activeOperation_ = CN0349Operation::None;
  lastOutcome_ = CN0349Outcome::Failed;
  state_ = CN0349State::Error;
  return false;
}

bool CN0349Manager::cancel() {
  if (temperatureContinuous_) return cancelTemperature();
  if (isEcRunning()) return cancelEc();
  if (isImpedanceRunning()) return cancelImpedance();
  if (isCalibrationRunning()) return cancelCalibration();
  if (!initialized_ || !isBusy()) return false;
  if (!impedance_.cancelOperation()) return false;
  cancelledByManager_ = true;
  return true;
}

const char* CN0349Manager::stateName(CN0349State state) {
  switch (state) {
    case CN0349State::NotInitialized: return "NOT_INITIALIZED";
    case CN0349State::Idle: return "IDLE";
    case CN0349State::RunningSingle: return "RUNNING_SINGLE";
    case CN0349State::RunningSweep: return "RUNNING_SWEEP";
    case CN0349State::RunningImpedance: return "RUNNING_IMPEDANCE";
    case CN0349State::RunningEc: return "RUNNING_EC";
    case CN0349State::Error: return "ERROR";
  }
  return "UNKNOWN";
}
const char* CN0349Manager::operationName(CN0349Operation operation) {
  switch (operation) {
    case CN0349Operation::None: return "NONE";
    case CN0349Operation::Single: return "SINGLE";
    case CN0349Operation::Sweep: return "SWEEP";
    case CN0349Operation::Impedance: return "IMPEDANCE";
    case CN0349Operation::Ec: return "EC";
  }
  return "UNKNOWN";
}
const char* CN0349Manager::outcomeName(CN0349Outcome outcome) {
  switch (outcome) {
    case CN0349Outcome::None: return "NONE";
    case CN0349Outcome::Complete: return "COMPLETE";
    case CN0349Outcome::Cancelled: return "CANCELLED";
    case CN0349Outcome::Failed: return "FAILED";
  }
  return "UNKNOWN";
}

const char* CN0349Manager::calibrationErrorName(CN0349CalibrationError error) {
  switch (error) {
    case CN0349CalibrationError::None: return "NONE";
    case CN0349CalibrationError::Busy: return "BUSY";
    case CN0349CalibrationError::InvalidArgument: return "INVALID_ARGUMENT";
    case CN0349CalibrationError::NotReady: return "NOT_READY";
    case CN0349CalibrationError::AcquisitionFailed: return "ACQUISITION_FAILED";
    case CN0349CalibrationError::Cancelled: return "CANCELLED";
  }
  return "UNKNOWN";
}

void CN0349Manager::synchronizeOutcome() {
  const bool appBusy = impedance_.isBusy();
  if (appBusy) {
    if (activeOperation_ == CN0349Operation::None) {
      activeOperation_ = impedance_.currentIsSweep() ? CN0349Operation::Sweep : CN0349Operation::Single;
      lastOperation_ = activeOperation_;
      lastOutcome_ = CN0349Outcome::None;
      resultOperationValid_ = true;
    }
    state_ = activeOperation_ == CN0349Operation::Sweep ? CN0349State::RunningSweep : CN0349State::RunningSingle;
    observedBusy_ = true;
    return;
  }
  if (observedBusy_ || activeOperation_ != CN0349Operation::None) {
    const auto appState = impedance_.measurementState();
    if (appState == ImpedanceApp::MeasurementState::Complete) {
      lastOutcome_ = CN0349Outcome::Complete;
      state_ = CN0349State::Idle;
    } else if (appState == ImpedanceApp::MeasurementState::Cancelled || cancelledByManager_ ||
               (appState == ImpedanceApp::MeasurementState::Idle && lastOutcome_ == CN0349Outcome::None)) {
      lastOutcome_ = CN0349Outcome::Cancelled;
      state_ = CN0349State::Idle;
    } else {
      lastOutcome_ = CN0349Outcome::Failed;
      state_ = CN0349State::Error;
    }
    activeOperation_ = CN0349Operation::None;
    cancelledByManager_ = false;
    observedBusy_ = false;
  } else if (state_ == CN0349State::Error && isReady()) {
    state_ = CN0349State::Idle;
  }
}

CN0349Status CN0349Manager::getStatus() const {
  CN0349Status status{};
  const auto hardware = impedance_.statusSnapshot();
  status.busy = isBusy();
  status.hardwareReady = hardware.hardwareReady;
  status.ad5934Communication = hardware.ad5934Communication;
  status.adg715Communication = hardware.adg715Communication;
  status.configurationVerified = hardware.configurationVerified;
  status.diagnosticComplete = hardware.diagnosticComplete;
  status.activeOperation = status.busy ? activeOperation_ : CN0349Operation::None;
  status.lastOperation = lastOperation_;
  status.lastOutcome = lastOutcome_;
  status.state = !initialized_ ? CN0349State::NotInitialized : status.busy ?
      (status.activeOperation == CN0349Operation::Impedance ? CN0349State::RunningImpedance : status.activeOperation == CN0349Operation::Ec ? CN0349State::RunningEc : status.activeOperation == CN0349Operation::Sweep ? CN0349State::RunningSweep : CN0349State::RunningSingle) : state_;
  const auto requested = getPreset();
  status.requestedPreset = requested;
  MuxApp::CalibrationInfo info{};
  status.requestedPresetValid = impedance_.presetInfo(toMuxPreset(requested), info);
  status.physicalMaskKnown = hardware.adg715MaskKnown;
  status.physicalMask = hardware.adg715Mask;
  if (status.physicalMaskKnown && impedance_.presetInfoForMask(status.physicalMask, info)) {
    status.appliedPresetValid = true;
    status.appliedPreset = static_cast<CN0349Preset>(info.id);
  }
  status.pointsValid = hardware.pointsCompleted;
  status.pointsExpected = hardware.pointsExpected;
  status.elapsedMs = hardware.elapsedMs;
  status.activeError = hardware.activeError;
  status.lastError = hardware.lastError;
  return status;
}

bool CN0349Manager::getSingleResult(CN0349RawResult& result) const {
  if (!resultOperationValid_ || lastOperation_ != CN0349Operation::Single || lastOutcome_ != CN0349Outcome::Complete) return false;
  ImpedanceApp::Result raw{};
  if (!impedance_.singleResult(raw)) return false;
  result.real = raw.real; result.imag = raw.imaginary;
  result.preset = static_cast<CN0349Preset>(raw.calibration.id);
  result.magnitude = static_cast<float>(raw.magnitude);
  result.phaseDeg = static_cast<float>(raw.phaseDegrees);
  result.requestedFrequencyHz = static_cast<float>(raw.frequencyHz);
  result.effectiveFrequencyHz = static_cast<float>(raw.effectiveFrequencyHz);
  result.elapsedMs = raw.elapsedMs; result.valid = raw.valid;
  return true;
}

bool CN0349Manager::getSweepResult(CN0349SweepResult& result) const {
  if (!resultOperationValid_ || lastOperation_ != CN0349Operation::Sweep || !impedance_.hasSweepResult()) return false;
  const auto& config = impedance_.activeSweepConfiguration();
  result.config = CN0349SweepConfig{static_cast<CN0349Preset>(impedance_.activeSweepPreset()),
      config.startHz, config.incrementHz, config.increments, impedance_.activeSweepSettling()};
  result.pointsValid = impedance_.sweepPointCount();
  result.pointsExpected = impedance_.sweepPointTarget();
  result.elapsedMs = impedance_.statusSnapshot().elapsedMs;
  result.outcome = lastOutcome_;
  for (uint16_t index = 0; index < result.pointsValid; ++index) {
    ImpedanceApp::SweepPoint raw{};
    if (!impedance_.sweepPoint(index, raw)) return false;
    CN0349SweepPoint& point = sweepSnapshot_[index];
    point.index = raw.index;
    point.requestedFrequencyHz = static_cast<float>(config.startHz + config.incrementHz * raw.index);
    point.effectiveFrequencyHz = static_cast<float>(raw.frequencyHz);
    point.real = raw.real; point.imag = raw.imaginary;
    point.magnitude = static_cast<float>(raw.magnitude);
    point.phaseDeg = static_cast<float>(raw.phaseDegrees); point.valid = raw.valid;
  }
  result.points = sweepSnapshot_;
  return true;
}

CN0349Diagnostics CN0349Manager::getDiagnostics() const {
  CN0349Diagnostics diagnostics{};
  diagnostics.subsystem = impedance_.statusSnapshot();
  MuxApp::CalibrationInfo info{};
  if (diagnostics.subsystem.adg715MaskKnown &&
      impedance_.presetInfoForMask(diagnostics.subsystem.adg715Mask, info)) {
    diagnostics.appliedPresetValid = true;
    diagnostics.appliedPreset = static_cast<CN0349Preset>(info.id);
  }
  return diagnostics;
}

bool CN0349Manager::runDiagnostics() {
  if (!initialized_ || isBusy()) return false;
  const bool ready = impedance_.runDiagnostics(busReady_);
  state_ = ready ? CN0349State::Idle : CN0349State::Error;
  return ready;
}
bool CN0349Manager::recover() {
  if (!initialized_ || isBusy() || isCalibrationRunning()) return false;
  const bool recovered = impedance_.requestRecovery(busReady_);
  if (recovered) state_ = CN0349State::Idle;
  else state_ = CN0349State::Error;
  return recovered;
}

bool CN0349Manager::isCalibrationRunning() const {
  return calibrationState_ == CN0349CalibrationState::Running;
}

bool CN0349Manager::prepareMeasurements() {
  if (!initialized_ || isBusy() || preparationPending_) return false;
  output_.println("Arranque automatico: cn0349 recover");
  if (!recover()) {
    output_.println("Arranque FAILED: recover no completado; no se inicia calibracion.");
    return false;
  }
  output_.println("Arranque automatico: cn0349 cal run all (2000 Hz)");
  const auto error = startCalibrationAll();
  if (error != CN0349CalibrationError::None) {
    output_.print("Arranque FAILED: no se inicio calibracion; error=");
    output_.println(calibrationErrorName(error));
    return false;
  }
  preparationPending_ = true;
  return true;
}

CN0349CalibrationError CN0349Manager::startCalibrationAll(uint32_t frequencyHz) {
  const auto error = startCalibration(CN0349Preset::CalHigh100R, frequencyHz);
  if (error == CN0349CalibrationError::None) {
    for (uint8_t i = 0; i < 4; ++i) calibrationSequence_[i] = static_cast<CN0349Preset>(i);
    calibrationAll_ = true;
  }
  return error;
}

bool CN0349Manager::calibrationCompatible(const CN0349Calibration& c) const {
  const auto& profile = impedance_.profile();
  return c.valid && c.frequencyHz == static_cast<float>(profile.frequencyHz) &&
      c.mclkHz == static_cast<float>(profile.mclkHz) &&
      c.excitationRange == profile.excitationRange && c.pgaGain == profile.pgaGain &&
      c.settlingCycles == profile.settlingCycles;
}

bool CN0349Manager::getCalibration(CN0349Preset preset, CN0349Calibration& result) const {
  const uint8_t index = static_cast<uint8_t>(preset);
  if (index >= 4 || !calibrations_[index].valid) return false;
  result = calibrations_[index];
  result.compatibleWithCurrentConfig = calibrationCompatible(result);
  return true;
}

CN0349CalibrationError CN0349Manager::startCalibration(CN0349Preset preset, uint32_t frequencyHz) {
  if (!initialized_) return calibrationError_ = CN0349CalibrationError::NotReady;
  if (isBusy() || isCalibrationRunning()) return calibrationError_ = CN0349CalibrationError::Busy;
  if (static_cast<uint8_t>(preset) >= 4 || frequencyHz == 0 || frequencyHz > 100000) {
    return calibrationError_ = CN0349CalibrationError::InvalidArgument;
  }
  if (!isReady()) return calibrationError_ = CN0349CalibrationError::NotReady;
  calibrationAll_ = false;
  calibrationSequence_[0] = preset;
  calibrationSequenceIndex_ = 0;
  calibrationStartedMs_ = millis();
  activePresetStartedMs_ = calibrationStartedMs_;
  calibrationSampleIndex_ = 0;
  calibrationCancelRequested_ = false;
  calibrationError_ = CN0349CalibrationError::None;
  lastCalibrationPreset_ = preset;
  calibrationState_ = CN0349CalibrationState::Running;
  if (!impedance_.configureSingle(preset == CN0349Preset::CalHigh100R ? MuxApp::CalibrationPreset::CalHigh100R :
      preset == CN0349Preset::CalHigh1K ? MuxApp::CalibrationPreset::CalHigh1K :
      preset == CN0349Preset::CalLow1K ? MuxApp::CalibrationPreset::CalLow1K : MuxApp::CalibrationPreset::CalLow10K,
      frequencyHz, impedance_.profile().settlingCycles)) {
    calibrationState_ = CN0349CalibrationState::Failed;
    return calibrationError_ = CN0349CalibrationError::InvalidArgument;
  }
  activeCalibrationPreset_ = preset;
  if (!startCalibrationSample()) {
    calibrationState_ = CN0349CalibrationState::Failed;
    return calibrationError_ = CN0349CalibrationError::AcquisitionFailed;
  }
  return calibrationError_ = CN0349CalibrationError::None;
}

bool CN0349Manager::startCalibrationSample() {
  if (!impedance_.requestSingle(busReady_)) return false;
  calibrationSampleInFlight_ = true;
  return true;
}

bool CN0349Manager::cancelCalibration() {
  if (!isCalibrationRunning()) return false;
  calibrationCancelRequested_ = true;
  if (impedance_.isBusy()) impedance_.cancelOperation();
  else {
    calibrationState_ = CN0349CalibrationState::Cancelled;
    calibrationSampleInFlight_ = false;
    calibrationError_ = CN0349CalibrationError::Cancelled;
  }
  return true;
}

void CN0349Manager::advanceCalibration() {
  if (calibrationCancelRequested_ && !impedance_.isBusy()) {
    calibrationState_ = CN0349CalibrationState::Cancelled;
    calibrationError_ = CN0349CalibrationError::Cancelled;
    calibrationSampleInFlight_ = false;
    return;
  }
  if (impedance_.isBusy() || !calibrationSampleInFlight_) return;
  calibrationSampleInFlight_ = false;
  if (impedance_.measurementState() != ImpedanceApp::MeasurementState::Complete) {
    calibrationState_ = calibrationCancelRequested_ ? CN0349CalibrationState::Cancelled : CN0349CalibrationState::Failed;
    calibrationError_ = calibrationCancelRequested_ ? CN0349CalibrationError::Cancelled : CN0349CalibrationError::AcquisitionFailed;
    return;
  }
  ImpedanceApp::Result raw{};
  if (!impedance_.singleResult(raw) || !raw.valid || raw.magnitude <= 0) {
    calibrationState_ = CN0349CalibrationState::Failed;
    calibrationError_ = CN0349CalibrationError::AcquisitionFailed;
    return;
  }
  if (calibrationSampleIndex_ == 0) {
    candidateCalibration_ = CN0349Calibration{};
    MuxApp::CalibrationInfo info{};
    if (!impedance_.presetInfo(toMuxPreset(activeCalibrationPreset_), info)) {
      calibrationState_ = CN0349CalibrationState::Failed;
      calibrationError_ = CN0349CalibrationError::InvalidArgument;
      return;
    }
    const auto& profile = impedance_.profile();
    candidateCalibration_.presetMask = info.mask;
    candidateCalibration_.frequencyHz = static_cast<float>(raw.frequencyHz);
    candidateCalibration_.effectiveFrequencyHz = static_cast<float>(raw.effectiveFrequencyHz);
    candidateCalibration_.mclkHz = static_cast<float>(profile.mclkHz);
    candidateCalibration_.rfbOhms = static_cast<float>(info.rfbOhms);
    candidateCalibration_.rcalOhms = static_cast<float>(info.rcalOhms);
    candidateCalibration_.excitationRange = profile.excitationRange;
    candidateCalibration_.pgaGain = profile.pgaGain;
    candidateCalibration_.settlingCycles = profile.settlingCycles;
    calibrationSumMagnitude_ = 0;
    calibrationSumPhaseSin_ = 0;
    calibrationSumPhaseCos_ = 0;
    calibrationMeanEffectiveHz_ = 0;
  }
  const uint8_t sample = calibrationSampleIndex_;
  candidateCalibration_.real[sample] = raw.real;
  candidateCalibration_.imag[sample] = raw.imaginary;
  candidateCalibration_.sampleMagnitude[sample] = static_cast<float>(raw.magnitude);
  candidateCalibration_.samplePhaseDeg[sample] = static_cast<float>(raw.phaseDegrees);
  calibrationSumMagnitude_ += static_cast<float>(raw.magnitude);
  const float radians = static_cast<float>(raw.phaseDegrees * 0.017453292519943295);
  calibrationSumPhaseSin_ += sinf(radians);
  calibrationSumPhaseCos_ += cosf(radians);
  calibrationMeanEffectiveHz_ += static_cast<float>(raw.effectiveFrequencyHz);
  ++calibrationSampleIndex_;
  candidateCalibration_.sampleCount = calibrationSampleIndex_;
  if (calibrationSampleIndex_ < CN0349_CALIBRATION_SAMPLES) {
    if (!startCalibrationSample()) {
      calibrationState_ = CN0349CalibrationState::Failed;
      calibrationError_ = CN0349CalibrationError::AcquisitionFailed;
    }
    return;
  }
  finishCalibrationPreset();
  if (!calibrationAll_) {
    calibrationState_ = CN0349CalibrationState::Complete;
    return;
  }
  ++calibrationSequenceIndex_;
  if (calibrationSequenceIndex_ >= 4) {
    calibrationState_ = CN0349CalibrationState::Complete;
    return;
  }
  activeCalibrationPreset_ = calibrationSequence_[calibrationSequenceIndex_];
  lastCalibrationPreset_ = activeCalibrationPreset_;
  activePresetStartedMs_ = millis();
  calibrationSampleIndex_ = 0;
  const uint8_t pi = static_cast<uint8_t>(activeCalibrationPreset_);
  const auto preset = static_cast<MuxApp::CalibrationPreset>(pi);
  if (!impedance_.configureSingle(preset, static_cast<uint32_t>(candidateCalibration_.frequencyHz), candidateCalibration_.settlingCycles) ||
      !startCalibrationSample()) {
    calibrationState_ = CN0349CalibrationState::Failed;
    calibrationError_ = CN0349CalibrationError::AcquisitionFailed;
  }
}

void CN0349Manager::finishCalibrationPreset() {
  float variance = 0;
  int32_t realSum = 0, imagSum = 0;
  for (uint8_t i = 0; i < CN0349_CALIBRATION_SAMPLES; ++i) {
    const float delta = candidateCalibration_.sampleMagnitude[i] - calibrationSumMagnitude_ / CN0349_CALIBRATION_SAMPLES;
    variance += delta * delta;
    realSum += candidateCalibration_.real[i];
    imagSum += candidateCalibration_.imag[i];
  }
  candidateCalibration_.meanMagnitude = calibrationSumMagnitude_ / CN0349_CALIBRATION_SAMPLES;
  candidateCalibration_.magnitudeStdDev = sqrtf(variance / (CN0349_CALIBRATION_SAMPLES - 1));
  candidateCalibration_.relativeVariationPct = candidateCalibration_.meanMagnitude > 0 ?
      100.0f * candidateCalibration_.magnitudeStdDev / candidateCalibration_.meanMagnitude : 0;
  candidateCalibration_.meanPhaseDeg = atan2f(calibrationSumPhaseSin_, calibrationSumPhaseCos_) * 57.29577951308232f;
  candidateCalibration_.gainFactorPerOhm = 1.0f / (candidateCalibration_.rcalOhms * candidateCalibration_.meanMagnitude);
  candidateCalibration_.effectiveFrequencyHz = calibrationMeanEffectiveHz_ / CN0349_CALIBRATION_SAMPLES;
  candidateCalibration_.representativeReal = static_cast<int16_t>(realSum / CN0349_CALIBRATION_SAMPLES);
  candidateCalibration_.representativeImag = static_cast<int16_t>(imagSum / CN0349_CALIBRATION_SAMPLES);
  candidateCalibration_.durationMs = millis() - activePresetStartedMs_;
  candidateCalibration_.timestampMs = millis();
  candidateCalibration_.calibrationId = nextCalibrationId_++;
  candidateCalibration_.valid = candidateCalibration_.meanMagnitude > 0 && candidateCalibration_.rcalOhms > 0;
  candidateCalibration_.compatibleWithCurrentConfig = calibrationCompatible(candidateCalibration_);
  if (candidateCalibration_.valid) calibrations_[static_cast<uint8_t>(activeCalibrationPreset_)] = candidateCalibration_;
}

bool CN0349Manager::clearCalibrations() {
  if (isCalibrationRunning() || isBusy()) return false;
  for (auto& calibration : calibrations_) calibration = CN0349Calibration{};
  return true;
}

void CN0349Manager::printConfig() {
  const auto& profile = impedance_.profile();
  const auto& sweep = impedance_.sweepConfiguration();
  output_.println("Configuracion CN-0349 (solicitada; sin conmutar hardware):");
  output_.print("Preset: "); output_.println(presetName(getPreset()));
  output_.print("Single: "); output_.print(profile.frequencyHz); output_.print(" Hz, settling "); output_.println(profile.settlingCycles);
  output_.print("Sweep: "); output_.print(sweep.startHz); output_.print(" Hz + "); output_.print(sweep.incrementHz);
  output_.print(" Hz x "); output_.print(sweep.increments); output_.print(" increments ("); output_.print(sweep.increments + 1); output_.println(" points)");
  const auto status = getStatus();
  output_.print("Maska leida: ");
  if (status.physicalMaskKnown) { output_.print("0x"); if (status.physicalMask < 16) output_.print('0'); output_.print(status.physicalMask, HEX); }
  else output_.print("desconocida");
  output_.print("; preset aplicado: "); output_.println(status.appliedPresetValid ? presetName(status.appliedPreset) : "desconocido");
}

void CN0349Manager::printStatus() {
  const auto status = getStatus();
  output_.println("Estado CN-0349:");
  output_.print("State: "); output_.println(stateName(status.state));
  output_.print("Busy: "); output_.println(status.busy ? "si" : "no");
  output_.print("Ready: "); output_.println(status.hardwareReady ? "si" : "no");
  output_.print("I2C AD5934/ADG715: "); output_.print(status.ad5934Communication ? "ACK" : "no confirmado"); output_.print(" / "); output_.println(status.adg715Communication ? "ACK" : "no confirmado");
  output_.print("Configuration/diagnostic: "); output_.print(status.configurationVerified ? "verified" : "unknown"); output_.print(" / "); output_.println(status.diagnosticComplete ? "complete" : "incomplete");
  output_.print("Active operation: "); output_.println(operationName(status.activeOperation));
  output_.print("Last operation/outcome: "); output_.print(operationName(status.lastOperation)); output_.print(" / "); output_.println(outcomeName(status.lastOutcome));
  output_.print("Preset solicitado: "); output_.println(status.requestedPresetValid ? presetName(status.requestedPreset) : "desconocido");
  output_.print("Preset aplicado: "); output_.println(status.appliedPresetValid ? presetName(status.appliedPreset) : "desconocido");
  output_.print("Mascara fisica observada: ");
  if (status.physicalMaskKnown) { output_.print("0x"); if (status.physicalMask < 16) output_.print('0'); output_.print(status.physicalMask, HEX); }
  else output_.print("desconocida");
  output_.println();
  output_.print("Points: "); output_.print(status.pointsValid); output_.print("/"); output_.println(status.pointsExpected);
  output_.print("Elapsed ms: "); output_.println(status.elapsedMs);
  output_.print("Errors active/last: "); output_.print(ImpedanceDiagnostics::errorName(status.activeError)); output_.print(" / "); output_.println(ImpedanceDiagnostics::errorName(status.lastError));
}

void CN0349Manager::printResult() {
  const auto status = getStatus();
  if (status.lastOperation == CN0349Operation::Single) {
    CN0349RawResult result{};
    if (!getSingleResult(result)) { output_.println("No hay resultado DFT unico valido."); return; }
    output_.println("Resultado single DFT RAW (no es impedancia):");
    output_.print("Preset usado: "); output_.println(presetName(result.preset));
    output_.print("Requested/effective Hz: "); output_.print(result.requestedFrequencyHz, 3); output_.print(" / "); output_.println(result.effectiveFrequencyHz, 3);
    output_.print("REAL/IMAG: "); output_.print(result.real); output_.print(" / "); output_.println(result.imag);
    output_.print("Magnitude/phase: "); output_.print(result.magnitude, 3); output_.print(" / "); output_.println(result.phaseDeg, 3);
    output_.print("Elapsed ms: "); output_.println(result.elapsedMs);
  } else if (status.lastOperation == CN0349Operation::Sweep) {
    CN0349SweepResult result{};
    if (!getSweepResult(result)) { output_.println("No hay resultados de barrido."); return; }
    output_.print("Sweep "); output_.print(outcomeName(result.outcome)); output_.print("; puntos ");
    output_.print(result.pointsValid); output_.print("/"); output_.println(result.pointsExpected);
    output_.println("Idx RequestedHz EffectiveHz REAL IMAG Magnitude PhaseDeg Valid");
    for (uint16_t i = 0; i < result.pointsValid; ++i) {
      const auto& point = result.points[i];
      output_.print(point.index); output_.print(" "); output_.print(point.requestedFrequencyHz, 3); output_.print(" ");
      output_.print(point.effectiveFrequencyHz, 3); output_.print(" "); output_.print(point.real); output_.print(" ");
      output_.print(point.imag); output_.print(" "); output_.print(point.magnitude, 3); output_.print(" ");
      output_.print(point.phaseDeg, 3); output_.print(" "); output_.println(point.valid ? "VALID" : "INVALID");
    }
    output_.println("Magnitud/fase son DFT RAW, no impedancia calibrada.");
  } else output_.println("No hay una operacion con resultado disponible.");
}

void CN0349Manager::printCalibrationHelp() {
  output_.println("cn0349 cal help|status|run <PRESET|all>|result|list|stop|clear");
  output_.println("5 adquisiciones DFT por preset a 2000 Hz; calibracion electronica, no de celda.");
}

void CN0349Manager::printCalibration(const CN0349Calibration& c) {
  const CN0349Preset preset = static_cast<CN0349Preset>(c.presetMask == 0x09 ? 0 : c.presetMask == 0x11 ? 1 : c.presetMask == 0x12 ? 2 : 3);
  output_.println("Calibracion electronica CN-0349:");
  output_.print("Preset: "); output_.println(presetName(preset));
  output_.print("RFB nominal: "); output_.print(c.rfbOhms, 1); output_.println(" ohm");
  output_.print("RCAL nominal (referencia del calculo): "); output_.print(c.rcalOhms, 1); output_.println(" ohm");
  output_.print("Frequency requested/effective: "); output_.print(c.frequencyHz, 3); output_.print(" / "); output_.print(c.effectiveFrequencyHz, 3); output_.println(" Hz");
  output_.print("Samples: "); output_.println(c.sampleCount);
  output_.print("Representative REAL/IMAG: "); output_.print(c.representativeReal); output_.print(" / "); output_.println(c.representativeImag);
  output_.print("Mean magnitude: "); output_.println(c.meanMagnitude, 3);
  output_.print("Magnitude stddev (sample): "); output_.println(c.magnitudeStdDev, 3);
  output_.print("Relative variation: "); output_.print(c.relativeVariationPct, 3); output_.println(" %");
  output_.print("Instrumental mean phase: "); output_.print(c.meanPhaseDeg, 3); output_.println(" deg");
  output_.print("Gain Factor: "); output_.println(c.gainFactorPerOhm, 9);
  output_.print("Excitation range/PGA: "); output_.print(c.excitationRange); output_.print(" / x"); output_.println(c.pgaGain);
  output_.print("Settling: "); output_.print(c.settlingCycles); output_.println(" cycles");
  output_.print("Calibration ID: "); output_.println(c.calibrationId);
  output_.print("Duration: "); output_.print(c.durationMs); output_.println(" ms");
  output_.print("Valid/config compatible: "); output_.print(c.valid ? "yes" : "no"); output_.print(" / "); output_.println(c.compatibleWithCurrentConfig ? "yes" : "no");
  output_.println("Gain Factor is not ohms. Phase is instrumental; no external cell calibration is implied.");
}

void CN0349Manager::printCalibrationStatus() {
  output_.print("Calibration state: ");
  switch (calibrationState_) {
    case CN0349CalibrationState::Idle: output_.print("IDLE"); break;
    case CN0349CalibrationState::Running: output_.print("RUNNING"); break;
    case CN0349CalibrationState::Complete: output_.print("COMPLETE"); break;
    case CN0349CalibrationState::Failed: output_.print("FAILED"); break;
    case CN0349CalibrationState::Cancelled: output_.print("CANCELLED"); break;
  }
  output_.print("; preset: "); output_.print(presetName(activeCalibrationPreset_));
  output_.print("; sample: "); output_.print(calibrationSampleIndex_); output_.print("/"); output_.println(CN0349_CALIBRATION_SAMPLES);
  output_.print("All sequence: "); output_.println(calibrationAll_ ? "yes" : "no");
  output_.print("Last calibration error: "); output_.println(calibrationErrorName(calibrationError_));
}

void CN0349Manager::printCalibrationList() {
  for (uint8_t index = 0; index < 4; ++index) {
    CN0349Calibration c{};
    output_.print(presetName(static_cast<CN0349Preset>(index))); output_.print(": ");
    if (!getCalibration(static_cast<CN0349Preset>(index), c)) output_.println("unavailable");
    else { output_.print("valid, GF="); output_.print(c.gainFactorPerOhm, 9); output_.print("; compatible="); output_.println(c.compatibleWithCurrentConfig ? "yes" : "no"); }
  }
}

void CN0349Manager::printCalibrationResult() {
  CN0349Calibration c{};
  if (!getCalibration(lastCalibrationPreset_, c)) { output_.println("No hay calibracion valida disponible."); return; }
  printCalibration(c);
}

bool CN0349Manager::dispatch(void* context, char* line, bool busReady, bool busBusy) {
  auto* manager = static_cast<CN0349Manager*>(context);
  manager->busReady_ = busReady;
  return manager->process(line, busReady, busBusy);
}

bool CN0349Manager::process(char* line, bool busReady, bool busBusy) {
  if (strncmp(line, "cn0349", 6) != 0 || (line[6] != '\0' && line[6] != ' ')) return false;
  char* args[6] = {};
  size_t count = 0;
  char* context = nullptr;
  for (char* token = strtok_r(line, " ", &context); token && count < 6; token = strtok_r(nullptr, " ", &context)) args[count++] = token;
  if (count >= 2 && strcmp(args[1], "impedance") == 0) return processImpedance(args, count, busReady, busBusy);
  if (count >= 2 && strcmp(args[1], "temperature") == 0) return processTemperature(args, count, busReady, busBusy);
  if (count >= 2 && strcmp(args[1], "ec") == 0) return processEc(args, count, busReady, busBusy);
  if (count >= 2 && strcmp(args[1], "conductivity") == 0) return processConductivity(args, count, busReady, busBusy);
  if (count == 1 || (count == 2 && strcmp(args[1], "help") == 0)) {
    output_.println("cn0349 help|status|config|preset list|preset set <NAME>|measure single|sweep|stop|result|diag|recover|cal ...|impedance ...|temperature ...|ec ...|conductivity ...");
    output_.println("Single/sweep: DFT RAW. impedance: impedancia aparente Pt100 en ohm; sin conductividad."); return true;
  }
  if (count == 2 && strcmp(args[1], "status") == 0) { printStatus(); return true; }
  if (count == 2 && strcmp(args[1], "config") == 0) { printConfig(); return true; }
  if (count == 2 && strcmp(args[1], "result") == 0) { printResult(); return true; }
  if (count == 3 && strcmp(args[1], "cal") == 0) {
    if (strcmp(args[2], "help") == 0) printCalibrationHelp();
    else if (strcmp(args[2], "status") == 0) printCalibrationStatus();
    else if (strcmp(args[2], "list") == 0) printCalibrationList();
    else if (strcmp(args[2], "result") == 0) printCalibrationResult();
    else if (strcmp(args[2], "stop") == 0) output_.println(cancelCalibration() ? "Cancelacion de calibracion solicitada." : "No hay calibracion activa.");
    else if (strcmp(args[2], "clear") == 0) output_.println(clearCalibrations() ? "Calibraciones RAM borradas." : "Rechazado: calibracion/adquisicion activa.");
    else output_.println("Uso: cn0349 cal help|status|list|result|stop|clear|run <PRESET|all>");
    return true;
  }
  if (count == 4 && strcmp(args[1], "cal") == 0 && strcmp(args[2], "run") == 0) {
    if (!busReady || busBusy || isBusy() || isCalibrationRunning()) { output_.println("Calibracion rechazada: hardware ocupado/no disponible."); return true; }
    if (strcmp(args[3], "all") == 0) {
      const auto result = startCalibrationAll();
      if (result == CN0349CalibrationError::None) {
        output_.println("Calibracion secuencial iniciada; cuatro presets, cinco muestras cada uno.");
      } else { output_.print("Calibracion rechazada; error code: "); output_.println(static_cast<uint8_t>(result)); }
    } else {
      CN0349Preset preset{};
      if (!parsePreset(args[3], preset)) output_.println("Preset invalido; no se inicio la calibracion.");
      else {
        const auto result = startCalibration(preset);
        if (result == CN0349CalibrationError::None) { output_.print("Calibracion iniciada: "); output_.println(presetName(preset)); }
        else { output_.print("Calibracion rechazada; error code: "); output_.println(static_cast<uint8_t>(result)); }
      }
    }
    return true;
  }
  if (count == 3 && strcmp(args[1], "preset") == 0 && strcmp(args[2], "list") == 0) {
    for (uint8_t i = 0; i < 4; ++i) {
      MuxApp::CalibrationInfo info{};
      if (impedance_.presetInfo(toMuxPreset(static_cast<CN0349Preset>(i)), info)) {
        output_.print(info.name); output_.print(" mask=0x"); if (info.mask < 16) output_.print('0');
        output_.print(info.mask, HEX); output_.print(" RFB="); output_.print(info.rfbOhms);
        output_.print(" RCAL="); output_.println(info.rcalOhms);
      }
    }
    return true;
  }
  if (count == 4 && strcmp(args[1], "preset") == 0 && strcmp(args[2], "set") == 0) {
    CN0349Preset preset{};
    if (isBusy()) output_.println("Rechazado: operacion activa.");
    else if (!parsePreset(args[3], preset) || !setPreset(preset)) output_.println("Preset no valido/no aceptado; no se conmutó hardware.");
    else { output_.print("Preset solicitado para la siguiente adquisicion: "); output_.println(presetName(preset)); }
    return true;
  }
  if (count == 3 && strcmp(args[1], "measure") == 0) {
    if (strcmp(args[2], "stop") == 0) {
      if (cancel()) output_.println("Cancelacion solicitada; los resultados validos se conservan.");
      else output_.println("No hay operacion activa.");
    } else if (strcmp(args[2], "single") == 0 || strcmp(args[2], "sweep") == 0) {
      if (!busReady || busBusy || isBusy()) { output_.println("Solicitud rechazada: subsistema/bus no disponible u ocupado."); return true; }
      const bool started = strcmp(args[2], "single") == 0 ? startSingle() : startSweep();
      output_.println(started ? "Solicitud aceptada; operacion asincrona iniciada." : "Solicitud rechazada; consulte cn0349 status y ad5934 errors.");
    } else output_.println("Uso: cn0349 measure single|sweep|stop");
    return true;
  }
  if (count == 2 && strcmp(args[1], "diag") == 0) {
    if (!busReady || busBusy || isBusy()) output_.println("Diagnostico rechazado: bus/subsistema ocupado o no disponible.");
    else { const bool ready = runDiagnostics(); output_.print("Manager diagnostic: "); output_.println(ready ? "READY" : "DEGRADED/UNKNOWN"); }
    return true;
  }
  if (count == 2 && strcmp(args[1], "recover") == 0) {
    if (!busReady || busBusy || isBusy()) output_.println("Recovery rechazado: bus/subsistema ocupado o no disponible.");
    else { const bool ok = recover(); output_.print("Manager recovery: "); output_.println(ok ? "COMPLETE" : "PARTIAL/FAILED"); }
    return true;
  }
  output_.println("Comando CN-0349 no valido. Use cn0349 help.");
  return true;
}
