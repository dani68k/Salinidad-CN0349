#include "CN0349Manager.h"
#include "ImpedanceMath.h"
#include <string.h>
#include <math.h>

const char* CN0349Manager::impedanceErrorName(CN0349ImpedanceError error) {
  switch (error) {
    case CN0349ImpedanceError::None: return "NONE";
    case CN0349ImpedanceError::Busy: return "BUSY";
    case CN0349ImpedanceError::NotReady: return "NOT_READY";
    case CN0349ImpedanceError::InvalidFrequency: return "INVALID_FREQUENCY";
    case CN0349ImpedanceError::InvalidCalibration: return "INVALID_CALIBRATION";
    case CN0349ImpedanceError::IncompatibleCalibration: return "INCOMPATIBLE_CALIBRATION";
    case CN0349ImpedanceError::AcquisitionFailed: return "ACQUISITION_FAILED";
    case CN0349ImpedanceError::InvalidData: return "INVALID_DATA";
    case CN0349ImpedanceError::Cancelled: return "CANCELLED";
  }
  return "UNKNOWN";
}
const char* CN0349Manager::impedanceStateName(CN0349ImpedanceState state) {
  switch (state) {
    case CN0349ImpedanceState::Idle: return "IDLE";
    case CN0349ImpedanceState::Running: return "RUNNING";
    case CN0349ImpedanceState::Complete: return "COMPLETE";
    case CN0349ImpedanceState::Failed: return "FAILED";
    case CN0349ImpedanceState::Cancelled: return "CANCELLED";
  }
  return "UNKNOWN";
}

bool CN0349Manager::impedanceCalibrationCompatible(const CN0349Calibration& c) const {
  return calibrationCompatibleForRfb(c, 100);
}

bool CN0349Manager::calibrationCompatibleForRfb(const CN0349Calibration& c, float rfbOhms) const {
  const auto& p = impedance_.profile();
  uint32_t code = 0;
  if ((rfbOhms != 100 && rfbOhms != 1000) || !AD5934::frequencyToCode(2000, p.mclkHz, code)) return false;
  // Same DDS code/clock, not an arbitrary frequency tolerance. The stored effective
  // frequency is a float mean of five identical DDS settings from milestone 2B.
  const float effective = static_cast<float>(static_cast<double>(code) * p.mclkHz / 134217728.0);
  return c.valid && (rfbOhms == 100 ? (c.presetMask == 0x09 || c.presetMask == 0x11) :
      (c.presetMask == 0x12 || c.presetMask == 0x22)) &&
      c.rfbOhms == rfbOhms && c.frequencyHz == 2000 &&
      c.mclkHz == p.mclkHz && CN0349Math::sameEffectiveFrequency(c.effectiveFrequencyHz, effective) &&
      p.externalClock && c.excitationRange == p.excitationRange &&
      c.pgaGain == p.pgaGain && c.settlingCycles == p.settlingCycles &&
      isfinite(c.gainFactorPerOhm) && c.gainFactorPerOhm > 0 && isfinite(c.meanPhaseDeg);
}

CN0349ImpedanceError CN0349Manager::selectImpedanceCalibration(CN0349Preset preset) {
  if (isBusy()) return impedanceRequestError_ = CN0349ImpedanceError::Busy;
  MuxApp::CalibrationInfo info{};
  if (!impedance_.presetInfo(toMuxPreset(preset), info))
    return impedanceRequestError_ = CN0349ImpedanceError::InvalidCalibration;
  if (info.rfbOhms != 100)
    return impedanceRequestError_ = CN0349ImpedanceError::IncompatibleCalibration;
  // Selection is allowed before calibration; start requires an available slot.
  impedanceCalibration_ = preset;
  return impedanceRequestError_ = CN0349ImpedanceError::None;
}

CN0349ImpedanceError CN0349Manager::startImpedance(bool quiet) {
  if (isBusy()) return impedanceRequestError_ = CN0349ImpedanceError::Busy;
  if (!initialized_ || !busReady_ || !isReady())
    return impedanceRequestError_ = CN0349ImpedanceError::NotReady;
  const auto p = impedance_.profile();
  uint32_t code = 0;
  if (p.frequencyHz != 2000 || !AD5934::frequencyToCode(p.frequencyHz, p.mclkHz, code))
    return impedanceRequestError_ = CN0349ImpedanceError::InvalidFrequency;
  CN0349Calibration c{};
  if (!getCalibration(impedanceCalibration_, c))
    return impedanceRequestError_ = CN0349ImpedanceError::InvalidCalibration;
  if (!impedanceCalibrationCompatible(c))
    return impedanceRequestError_ = CN0349ImpedanceError::IncompatibleCalibration;
  impedanceCalibrationSnapshot_ = c;
  impedanceQuiet_ = quiet;
  impedanceResult_ = CN0349ImpedanceResult{};
  auto& r = impedanceResult_;
  r.calibrationPreset = impedanceCalibration_;
  r.calibrationId = c.calibrationId;
  r.calibrationMask = c.presetMask;
  r.gainFactor = c.gainFactorPerOhm;
  r.calibrationPhaseDeg = c.meanPhaseDeg;
  r.calibrationEffectiveFrequencyHz = c.effectiveFrequencyHz;
  r.frequencyHz = static_cast<float>(p.frequencyHz);
  r.mclkHz = p.mclkHz;
  r.excitationRange = p.excitationRange;
  r.pgaGain = p.pgaGain;
  r.settlingCycles = p.settlingCycles;
  r.state = CN0349ImpedanceState::Running;
  impedanceCancelRequested_ = false;
  impedanceStartedMs_ = millis();
  activeOperation_ = lastOperation_ = CN0349Operation::Impedance;
  lastOutcome_ = CN0349Outcome::None;
  resultOperationValid_ = false;
  state_ = CN0349State::RunningImpedance;
  const bool started = impedance_.requestPt100(busReady_, quiet);
  if (!started) r.error = CN0349ImpedanceError::AcquisitionFailed;
  // A rejected preflight may still have scheduled standby recovery. Keep the
  // manager busy until that recovery finishes; never publish COMPLETE early.
  if (!impedance_.isBusy()) advanceImpedance();
  return impedanceRequestError_ = started ? CN0349ImpedanceError::None : CN0349ImpedanceError::AcquisitionFailed;
}

bool CN0349Manager::cancelImpedance() {
  if (!isImpedanceRunning() || !impedance_.cancelOperation()) return false;
  impedanceCancelRequested_ = true;
  return true;
}

void CN0349Manager::advanceImpedance() {
  if (impedance_.isBusy()) return;
  auto& r = impedanceResult_;
  r.elapsedMs = millis() - impedanceStartedMs_;
  r.hardwareError = impedance_.statusSnapshot().lastError;
  const auto appState = impedance_.measurementState();
  const bool complete = appState == ImpedanceApp::MeasurementState::Complete;
  const bool safelyCancelled = appState == ImpedanceApp::MeasurementState::Idle ||
      appState == ImpedanceApp::MeasurementState::Cancelled;
  r.state = CN0349ImpedanceState::Failed;
  if (safelyCancelled && (impedanceCancelRequested_ || r.error == CN0349ImpedanceError::None)) {
    r.state = CN0349ImpedanceState::Cancelled;
    r.error = CN0349ImpedanceError::Cancelled;
  } else if (!complete || r.error != CN0349ImpedanceError::None) {
    r.error = CN0349ImpedanceError::AcquisitionFailed;
  } else {
    ImpedanceApp::Result raw{};
    if (!impedance_.singleResult(raw) || !raw.valid) r.error = CN0349ImpedanceError::InvalidData;
    else {
      r.real = raw.real; r.imag = raw.imaginary;
      r.magnitude = static_cast<float>(raw.magnitude);
      r.rawPhaseDeg = static_cast<float>(raw.phaseDegrees);
      r.effectiveFrequencyHz = static_cast<float>(raw.effectiveFrequencyHz);
      if (raw.calibration.mask != r.measurementMask || raw.calibration.rfbOhms != 100 ||
          raw.frequencyHz != r.frequencyHz || !CN0349Math::sameEffectiveFrequency(r.effectiveFrequencyHz, r.calibrationEffectiveFrequencyHz) ||
          !impedanceCalibrationCompatible(impedanceCalibrationSnapshot_) ||
          !CN0349Math::apparentImpedance(r.magnitude, r.gainFactor, r.rawPhaseDeg,
              r.calibrationPhaseDeg, r.apparentImpedanceOhms, r.correctedPhaseDeg))
        r.error = CN0349ImpedanceError::InvalidData;
      else {
        r.valid = true;
        r.state = CN0349ImpedanceState::Complete;
        // lastError is historical; successful completion already verified standby.
        r.hardwareError = ImpedanceDiagnostics::ErrorCode::None;
        lastValidImpedanceResult_ = r;
      }
    }
  }
  lastOutcome_ = r.valid ? CN0349Outcome::Complete :
      r.state == CN0349ImpedanceState::Cancelled ? CN0349Outcome::Cancelled : CN0349Outcome::Failed;
  state_ = r.state == CN0349ImpedanceState::Failed ? CN0349State::Error : CN0349State::Idle;
  activeOperation_ = CN0349Operation::None;
  observedBusy_ = false;
  if (!impedanceQuiet_ || !r.valid) {
    output_.print("Pt100 impedance: "); output_.print(impedanceStateName(r.state));
    output_.print("; error="); output_.println(impedanceErrorName(r.error));
  }
}

bool CN0349Manager::getLastValidImpedanceResult(CN0349ImpedanceResult& result) const {
  if (!lastValidImpedanceResult_.valid) return false;
  result = lastValidImpedanceResult_;
  return true;
}

void CN0349Manager::printImpedanceConfig() {
  output_.println("MEASURE_PT100 / 0x41 (RTD), RFB=100 ohm; 2000 Hz exclusivamente.");
  output_.print("Calibracion seleccionada: "); output_.println(presetName(impedanceCalibration_));
  const auto& p = impedance_.profile();
  output_.print("Perfil actual Hz/MCLK/Range/PGA/settling: ");
  output_.print(p.frequencyHz); output_.print(" / "); output_.print(p.mclkHz); output_.print(" / ");
  output_.print(p.excitationRange); output_.print(" / "); output_.print(p.pgaGain); output_.print(" / "); output_.println(p.settlingCycles);
  output_.println("Z_aparente=1/(GF*M); sin correccion de ruta ni temperatura.");
  output_.println("No detecta fiablemente saturacion/senal insuficiente; validez numerica no demuestra exactitud.");
}
void CN0349Manager::printImpedanceStatus() {
  output_.print("Impedance state: "); output_.println(impedanceStateName(impedanceResult_.state));
  output_.print("Attempt error: "); output_.println(impedanceErrorName(impedanceResult_.error));
  output_.print("Last request error: "); output_.println(impedanceErrorName(impedanceRequestError_));
  output_.print("Cancel pending: "); output_.println(impedanceCancelRequested_ && isImpedanceRunning() ? "yes" : "no");
  output_.print("Subsystem busy: "); output_.println(isBusy() ? "yes" : "no");
}
void CN0349Manager::printImpedanceResult(const CN0349ImpedanceResult& r) {
  output_.println("MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm");
  output_.print("State/valid/error: "); output_.print(impedanceStateName(r.state)); output_.print(" / ");
  output_.print(r.valid ? "VALID" : "INVALID"); output_.print(" / "); output_.println(impedanceErrorName(r.error));
  output_.print("Calibration/mask/ID: "); output_.print(presetName(r.calibrationPreset)); output_.print(" / 0x");
  output_.print(r.calibrationMask, HEX); output_.print(" / "); output_.println(r.calibrationId);
  output_.print("Requested/effective/calibration effective Hz: "); output_.print(r.frequencyHz, 3); output_.print(" / ");
  output_.print(r.effectiveFrequencyHz, 6); output_.print(" / "); output_.println(r.calibrationEffectiveFrequencyHz, 6);
  output_.print("MCLK/Range/PGA/settling: "); output_.print(r.mclkHz); output_.print(" / "); output_.print(r.excitationRange);
  output_.print(" / "); output_.print(r.pgaGain); output_.print(" / "); output_.println(r.settlingCycles);
  output_.print("REAL/IMAG: "); output_.print(r.real); output_.print(" / "); output_.println(r.imag);
  output_.print("DFT magnitude (code): "); output_.println(r.magnitude, 6);
  output_.print("DFT/calibration phase deg: "); output_.print(r.rawPhaseDeg, 6); output_.print(" / "); output_.println(r.calibrationPhaseDeg, 6);
  output_.print("Phase corrected instrumentally deg (not validated physical phase): "); output_.println(r.correctedPhaseDeg, 6);
  output_.print("Gain Factor (1/(ohm*DFT code)): "); output_.println(r.gainFactor, 12);
  output_.print("Apparent impedance ohm: "); if (r.valid) output_.println(r.apparentImpedanceOhms, 6); else output_.println("unavailable");
  output_.print("Elapsed ms: "); output_.println(r.elapsedMs);
  output_.print("Hardware error (historical on failure): "); output_.println(ImpedanceDiagnostics::errorName(r.hardwareError));
  output_.println("Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.");
}

bool CN0349Manager::processImpedance(char** args, size_t count, bool busReady, bool busBusy) {
  if (count == 3 && strcmp(args[2], "config") == 0) printImpedanceConfig();
  else if (count == 3 && strcmp(args[2], "status") == 0) printImpedanceStatus();
  else if (count == 3 && strcmp(args[2], "result") == 0) {
    printImpedanceStatus();
    printImpedanceResult(impedanceResult_);
    if (!impedanceResult_.valid && lastValidImpedanceResult_.valid) {
      output_.println("Ultimo resultado valido anterior (no pertenece al intento actual):");
      printImpedanceResult(lastValidImpedanceResult_);
    }
  } else if (count == 4 && strcmp(args[2], "cal") == 0 && strcmp(args[3], "list") == 0) {
    for (uint8_t i = 0; i < 4; ++i) {
      const auto preset = static_cast<CN0349Preset>(i);
      CN0349Calibration c{};
      output_.print(presetName(preset)); output_.print(": ");
      if (i >= 2) output_.println("INCOMPATIBLE RFB=1000 ohm");
      else if (!getCalibration(preset, c)) output_.println("UNAVAILABLE; ejecutar cn0349 cal run <PRESET>");
      else {
        output_.print(impedanceCalibrationCompatible(c) ? "COMPATIBLE" : "INCOMPATIBLE CONFIG");
        output_.print("; ID="); output_.print(c.calibrationId); output_.print("; GF="); output_.println(c.gainFactorPerOhm, 12);
      }
    }
  } else if (count == 5 && strcmp(args[2], "cal") == 0 && strcmp(args[3], "set") == 0) {
    CN0349Preset preset{};
    if (!parsePreset(args[4], preset)) impedanceRequestError_ = CN0349ImpedanceError::InvalidCalibration;
    else selectImpedanceCalibration(preset);
    output_.print("Calibration selection: "); output_.println(impedanceErrorName(impedanceRequestError_));
  } else if (count == 3 && strcmp(args[2], "start") == 0) {
    if (isBusy() || busBusy) impedanceRequestError_ = CN0349ImpedanceError::Busy;
    else if (!busReady) impedanceRequestError_ = CN0349ImpedanceError::NotReady;
    else startImpedance();
    output_.print("Impedance request: "); output_.println(impedanceErrorName(impedanceRequestError_));
  } else if (count == 3 && strcmp(args[2], "stop") == 0)
    output_.println(cancelImpedance() ? "Cancelacion solicitada; esperando standby." : "No hay medida Pt100 cancelable.");
  else {
    output_.println("cn0349 impedance help|config|cal list|cal set <CAL_HIGH_100R|CAL_HIGH_1K>|start|status|result|stop");
    output_.println("Seleccion manual; start exige calibracion RAM valida y configuracion compatible a 2000 Hz.");
  }
  return true;
}
