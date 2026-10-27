#include "CN0349Manager.h"
#include "ImpedanceMath.h"
#include "../../include/config.h"
#include <math.h>
#include <string.h>

namespace {
uint8_t ecMask(CN0349EcRange range) { return range == CN0349EcRange::High ? 0x81 : 0x82; }
float ecRfb(CN0349EcRange range) { return range == CN0349EcRange::High ? 100.0f : 1000.0f; }
const char* ecName(CN0349EcRange range) { return range == CN0349EcRange::High ? "HIGH" : "LOW"; }
const char* conductivityErrorName(Conductivity::Error error) {
  switch (error) {
    case Conductivity::Error::None: return "NONE";
    case Conductivity::Error::NotRun: return "NOT_RUN";
    case Conductivity::Error::InvalidModel: return "INVALID_MODEL";
    case Conductivity::Error::InvalidImpedance: return "INVALID_IMPEDANCE";
    case Conductivity::Error::InvalidCellConstant: return "INVALID_K";
    case Conductivity::Error::InvalidR2: return "INVALID_R2";
    case Conductivity::Error::NonPositiveCellResistance: return "NON_POSITIVE_CELL_RESISTANCE";
    case Conductivity::Error::Overflow: return "OVERFLOW";
  }
  return "UNKNOWN";
}
const char* ad5934FinalName(ImpedanceApp::MeasurementState state) {
  switch (state) {
    case ImpedanceApp::MeasurementState::Idle: return "IDLE";
    case ImpedanceApp::MeasurementState::Complete: return "COMPLETE";
    case ImpedanceApp::MeasurementState::Cancelled: return "CANCELLED";
    case ImpedanceApp::MeasurementState::Error: return "ERROR";
    default: return "IN_PROGRESS";
  }
}
}

CN0349ImpedanceError CN0349Manager::selectEcRange(CN0349EcRange range) {
  if (isBusy()) return ecRequestError_ = CN0349ImpedanceError::Busy;
  if (range != CN0349EcRange::High && range != CN0349EcRange::Low)
    return ecRequestError_ = CN0349ImpedanceError::InvalidData;
  ecRange_ = range;
  return ecRequestError_ = CN0349ImpedanceError::None;
}

CN0349ImpedanceError CN0349Manager::selectEcCalibration(CN0349Preset preset) {
  if (isBusy()) return ecRequestError_ = CN0349ImpedanceError::Busy;
  MuxApp::CalibrationInfo info{};
  if (!impedance_.presetInfo(toMuxPreset(preset), info))
    return ecRequestError_ = CN0349ImpedanceError::InvalidCalibration;
  if (info.rfbOhms != ecRfb(ecRange_))
    return ecRequestError_ = CN0349ImpedanceError::IncompatibleCalibration;
  ecCalibration_ = preset;
  return ecRequestError_ = CN0349ImpedanceError::None;
}

CN0349ImpedanceError CN0349Manager::startEc() {
  if (isBusy()) return ecRequestError_ = CN0349ImpedanceError::Busy;
  if (!initialized_ || !busReady_ || !isReady())
    return ecRequestError_ = CN0349ImpedanceError::NotReady;
  const auto p = impedance_.profile();
  uint32_t code = 0;
  if (p.frequencyHz != 2000 || !AD5934::frequencyToCode(p.frequencyHz, p.mclkHz, code))
    return ecRequestError_ = CN0349ImpedanceError::InvalidFrequency;
  MuxApp::CalibrationInfo route{};
  if (!impedance_.ecInfo(ecMask(ecRange_), route))
    return ecRequestError_ = CN0349ImpedanceError::InvalidData;
  MuxApp::CalibrationInfo selected{};
  if (!impedance_.presetInfo(toMuxPreset(ecCalibration_), selected))
    return ecRequestError_ = CN0349ImpedanceError::InvalidCalibration;
  if (selected.rfbOhms != route.rfbOhms)
    return ecRequestError_ = CN0349ImpedanceError::IncompatibleCalibration;
  CN0349Calibration c{};
  if (!getCalibration(ecCalibration_, c))
    return ecRequestError_ = CN0349ImpedanceError::InvalidCalibration;
  if (!calibrationCompatibleForRfb(c, route.rfbOhms))
    return ecRequestError_ = CN0349ImpedanceError::IncompatibleCalibration;

  ecCalibrationSnapshot_ = c;
  ecResult_ = CN0349EcResult{};
  ecResult_.range = ecRange_;
  ecResult_.rfbOhms = route.rfbOhms;
  auto& r = ecResult_.acquisition;
  r.measurementMask = route.mask;
  r.calibrationPreset = ecCalibration_;
  r.calibrationMask = c.presetMask;
  r.calibrationId = c.calibrationId;
  r.gainFactor = c.gainFactorPerOhm;
  r.calibrationPhaseDeg = c.meanPhaseDeg;
  r.calibrationEffectiveFrequencyHz = c.effectiveFrequencyHz;
  r.frequencyHz = static_cast<float>(p.frequencyHz);
  r.mclkHz = p.mclkHz;
  r.excitationRange = p.excitationRange;
  r.pgaGain = p.pgaGain;
  r.settlingCycles = p.settlingCycles;
  r.state = CN0349ImpedanceState::Running;
  ecCancelRequested_ = false;
  ecStartedMs_ = millis();
  activeOperation_ = lastOperation_ = CN0349Operation::Ec;
  lastOutcome_ = CN0349Outcome::None;
  resultOperationValid_ = false;
  state_ = CN0349State::RunningEc;
  const bool started = impedance_.requestEc(busReady_, route.mask);
  if (!started) r.error = CN0349ImpedanceError::AcquisitionFailed;
  if (!impedance_.isBusy()) advanceEc();
  return ecRequestError_ = started ? CN0349ImpedanceError::None : CN0349ImpedanceError::AcquisitionFailed;
}

bool CN0349Manager::cancelEc() {
  if (!isEcRunning() || !impedance_.cancelOperation()) return false;
  ecCancelRequested_ = true;
  return true;
}

void CN0349Manager::advanceEc() {
  if (impedance_.isBusy()) return;
  auto& result = ecResult_;
  auto& r = result.acquisition;
  r.elapsedMs = millis() - ecStartedMs_;
  const auto status = impedance_.statusSnapshot();
  r.hardwareError = status.lastError;
  const auto appState = impedance_.measurementState();
  result.ad5934FinalState = appState;
  result.standbyVerified = (appState == ImpedanceApp::MeasurementState::Complete ||
      appState == ImpedanceApp::MeasurementState::Cancelled) && status.hardwareReady;
  const bool complete = appState == ImpedanceApp::MeasurementState::Complete;
  const bool safelyCancelled = appState == ImpedanceApp::MeasurementState::Idle ||
      appState == ImpedanceApp::MeasurementState::Cancelled;
  r.state = CN0349ImpedanceState::Failed;
  if (safelyCancelled && (ecCancelRequested_ || r.error == CN0349ImpedanceError::None)) {
    r.state = CN0349ImpedanceState::Cancelled;
    r.error = CN0349ImpedanceError::Cancelled;
  } else if (!complete || r.error != CN0349ImpedanceError::None || !result.standbyVerified) {
    r.error = CN0349ImpedanceError::AcquisitionFailed;
  } else {
    ImpedanceApp::Result raw{};
    if (!impedance_.singleResult(raw) || !raw.valid) r.error = CN0349ImpedanceError::InvalidData;
    else {
      r.real = raw.real; r.imag = raw.imaginary;
      r.magnitude = static_cast<float>(raw.magnitude);
      r.rawPhaseDeg = static_cast<float>(raw.phaseDegrees);
      r.effectiveFrequencyHz = static_cast<float>(raw.effectiveFrequencyHz);
      if (raw.calibration.mask != r.measurementMask || raw.calibration.rfbOhms != result.rfbOhms ||
          raw.frequencyHz != r.frequencyHz ||
          !CN0349Math::sameEffectiveFrequency(r.effectiveFrequencyHz, r.calibrationEffectiveFrequencyHz) ||
          !calibrationCompatibleForRfb(ecCalibrationSnapshot_, result.rfbOhms) ||
          !CN0349Math::apparentImpedance(r.magnitude, r.gainFactor, r.rawPhaseDeg,
              r.calibrationPhaseDeg, r.apparentImpedanceOhms, r.correctedPhaseDeg))
        r.error = CN0349ImpedanceError::InvalidData;
      else {
        r.valid = true;
        r.state = CN0349ImpedanceState::Complete;
        r.hardwareError = ImpedanceDiagnostics::ErrorCode::None;
        lastValidEcResult_ = result;
      }
    }
  }
  lastOutcome_ = r.valid ? CN0349Outcome::Complete :
      r.state == CN0349ImpedanceState::Cancelled ? CN0349Outcome::Cancelled : CN0349Outcome::Failed;
  state_ = r.state == CN0349ImpedanceState::Failed ? CN0349State::Error : CN0349State::Idle;
  activeOperation_ = CN0349Operation::None;
  observedBusy_ = false;
  output_.print("EC impedance: "); output_.print(impedanceStateName(r.state));
  output_.print("; error="); output_.println(impedanceErrorName(r.error));
  if (conductivityRunning_) finishConductivity();
}

bool CN0349Manager::selectConductivityModel(Conductivity::Model model) {
  if (isBusy()) return false;
  // The physical board topology has not been verified. Only RAW is available.
  if (model != Conductivity::Model::Raw) return false;
  conductivityModel_ = model;
  return true;
}

CN0349ImpedanceError CN0349Manager::startConductivity() {
  if (isBusy()) return conductivityRequestError_ = CN0349ImpedanceError::Busy;
  const auto previous = conductivityResult_;
  conductivityResult_ = CN0349ConductivityResult{};
  conductivityResult_.conversion.model = conductivityModel_;
  conductivityResult_.conversion.nominalKPerCm = Config::EC_NOMINAL_CELL_CONSTANT_PER_CM;
  conductivityResult_.conversion.r2Ohms = Config::EC_NOMINAL_R2_OHMS;
  conductivityRunning_ = true;
  const auto error = startEc();
  conductivityRequestError_ = error;
  if (error != CN0349ImpedanceError::None && conductivityRunning_) {
    conductivityRunning_ = false;
    conductivityResult_ = previous;  // Rejected preflight did not start a new acquisition.
  }
  return error;
}

bool CN0349Manager::cancelConductivity() {
  return conductivityRunning_ && cancelEc();
}

void CN0349Manager::finishConductivity() {
  conductivityResult_.ec = ecResult_;
  const auto& acquisition = ecResult_.acquisition;
  if (acquisition.valid && acquisition.state == CN0349ImpedanceState::Complete &&
      acquisition.error == CN0349ImpedanceError::None &&
      acquisition.hardwareError == ImpedanceDiagnostics::ErrorCode::None && ecResult_.standbyVerified) {
    conductivityResult_.conversion = Conductivity::convert(acquisition.apparentImpedanceOhms,
        Config::EC_NOMINAL_CELL_CONSTANT_PER_CM, conductivityResult_.conversion.model,
        Config::EC_NOMINAL_R2_OHMS);
    conductivityResult_.valid = conductivityResult_.conversion.valid;
  }
  conductivityRunning_ = false;
}

void CN0349Manager::printConductivityResult(const CN0349ConductivityResult& result) {
  output_.println("CONDUCTIVITY - EXPERIMENTAL / NOT CALIBRATED");
  printEcResult(result.ec);
  output_.print("Model: "); output_.println(result.conversion.model == Conductivity::Model::Raw ?
      "RAW" : "SERIES_R2_EXPERIMENTAL");
  output_.print("R2 nominal ohm: "); output_.println(result.conversion.r2Ohms, 0);
  output_.print("K nominal cm^-1: "); output_.println(result.conversion.nominalKPerCm, 3);
  output_.print("Conversion error: "); output_.println(conductivityErrorName(result.conversion.error));
  if (!result.valid) {
    output_.println("Estimated cell resistance/conductivity: unavailable (acquisition or conversion invalid).");
    return;
  }
  output_.print("Estimated cell resistance ohm: "); output_.println(result.conversion.cellResistanceOhms, 6);
  output_.print("Estimated conductivity uS/cm: "); output_.println(result.conversion.microSiemensPerCm, 6);
  output_.print("Estimated conductivity mS/cm: "); output_.println(result.conversion.milliSiemensPerCm, 6);
  output_.println("Magnitude-based approximation; not Re(Z), not compensated to 25 C.");
}

bool CN0349Manager::processConductivity(char** args, size_t count, bool busReady, bool busBusy) {
  if (count == 3 && strcmp(args[2], "config") == 0) {
    output_.print("EC route/mask/RFB ohm: "); output_.print(ecName(ecRange_));
    output_.print(" / 0x"); output_.print(ecMask(ecRange_), HEX);
    output_.print(" / "); output_.println(ecRfb(ecRange_), 0);
    output_.print("Calibration selected: "); output_.println(presetName(ecCalibration_));
    const auto& p = impedance_.profile();
    output_.print("Hz/MCLK/Range/PGA/settling: "); output_.print(p.frequencyHz);
    output_.print(" / "); output_.print(p.mclkHz);
    output_.print(" / "); output_.print(p.excitationRange);
    output_.print(" / "); output_.print(p.pgaGain);
    output_.print(" / "); output_.println(p.settlingCycles);
    output_.print("K nominal cm^-1 / R2 nominal ohm: ");
    output_.print(Config::EC_NOMINAL_CELL_CONSTANT_PER_CM, 3);
    output_.print(" / "); output_.println(Config::EC_NOMINAL_R2_OHMS, 0);
    output_.println("Model: RAW (series R2 model unavailable until physical topology verification).");
  } else if (count == 4 && strcmp(args[2], "model") == 0 && strcmp(args[3], "list") == 0) {
    output_.println("RAW: available; SERIES_R2_EXPERIMENTAL: unavailable (physical topology unverified).");
  } else if (count == 5 && strcmp(args[2], "model") == 0 && strcmp(args[3], "set") == 0) {
    if (strcmp(args[4], "raw") == 0) output_.println(selectConductivityModel(Conductivity::Model::Raw) ?
        "Conductivity model: RAW" : "Conductivity model: BUSY");
    else if (strcmp(args[4], "series_r2_experimental") == 0)
      output_.println("Conductivity model rejected: R2 topology not physically verified; RAW remains selected.");
    else output_.println("Conductivity model: INVALID_MODEL");
  } else if (count == 3 && strcmp(args[2], "start") == 0) {
    if (busBusy) conductivityRequestError_ = CN0349ImpedanceError::Busy;
    else if (!busReady) conductivityRequestError_ = CN0349ImpedanceError::NotReady;
    else startConductivity();
    output_.print("Conductivity request: "); output_.println(impedanceErrorName(conductivityRequestError_));
  } else if (count == 3 && strcmp(args[2], "stop") == 0) {
    output_.println(cancelConductivity() ? "Conductivity cancellation requested; waiting for standby." :
        "No active conductivity measurement.");
  } else if (count == 3 && strcmp(args[2], "status") == 0) {
    output_.print("Conductivity state/error: ");
    output_.print(impedanceStateName(conductivityRunning_ ? ecResult_.acquisition.state :
        conductivityResult_.ec.acquisition.state));
    output_.print(" / "); output_.println(impedanceErrorName(conductivityRunning_ ?
        ecResult_.acquisition.error : conductivityResult_.ec.acquisition.error));
    output_.print("Conductivity last request error: "); output_.println(impedanceErrorName(conductivityRequestError_));
    output_.print("Conductivity valid/busy: "); output_.print(conductivityResult_.valid ? "yes" : "no");
    output_.print(" / "); output_.println(conductivityRunning_ ? "yes" : "no");
    output_.print("Conversion error: "); output_.println(conductivityErrorName(conductivityResult_.conversion.error));
  } else if (count == 3 && strcmp(args[2], "result") == 0) {
    if (!conductivityResult_.ec.acquisition.valid &&
        conductivityResult_.ec.acquisition.state == CN0349ImpedanceState::Idle)
      output_.println("No conductivity result available.");
    else printConductivityResult(conductivityResult_);
  } else {
    output_.println("cn0349 conductivity help|config|model list|model set RAW|SERIES_R2_EXPERIMENTAL|start|status|result|stop");
    output_.println("Choose EC route/calibration explicitly with cn0349 ec commands; RAW only until R2 topology verification.");
  }
  return true;
}

bool CN0349Manager::getLastValidEcResult(CN0349EcResult& result) const {
  if (!lastValidEcResult_.acquisition.valid) return false;
  result = lastValidEcResult_;
  return true;
}

void CN0349Manager::printEcResult(const CN0349EcResult& result) {
  const auto& r = result.acquisition;
  output_.println("EC PROBE - EXPERIMENTAL / NOT ABSOLUTELY CALIBRATED");
  output_.print("State/valid/error/hardware: "); output_.print(impedanceStateName(r.state));
  output_.print(" / "); output_.print(r.valid ? "VALID" : "INVALID");
  output_.print(" / "); output_.print(impedanceErrorName(r.error));
  output_.print(" / "); output_.println(ImpedanceDiagnostics::errorName(r.hardwareError));
  output_.print("EC route/mask/RFB ohm: "); output_.print(ecName(result.range));
  output_.print(" / 0x"); output_.print(r.measurementMask, HEX);
  output_.print(" / "); output_.println(result.rfbOhms, 0);
  output_.print("Calibration/mask/ID/GF: "); output_.print(presetName(r.calibrationPreset));
  output_.print(" / 0x"); output_.print(r.calibrationMask, HEX);
  output_.print(" / "); output_.print(r.calibrationId);
  output_.print(" / "); output_.println(r.gainFactor, 12);
  output_.print("Requested/effective/calibration effective Hz: "); output_.print(r.frequencyHz, 3);
  output_.print(" / "); output_.print(r.effectiveFrequencyHz, 6);
  output_.print(" / "); output_.println(r.calibrationEffectiveFrequencyHz, 6);
  output_.print("MCLK/Range/PGA/settling: "); output_.print(r.mclkHz);
  output_.print(" / "); output_.print(r.excitationRange);
  output_.print(" / "); output_.print(r.pgaGain);
  output_.print(" / "); output_.println(r.settlingCycles);
  output_.print("REAL/IMAG/DFT magnitude: "); output_.print(r.real);
  output_.print(" / "); output_.print(r.imag);
  output_.print(" / "); output_.println(r.magnitude, 6);
  output_.print("Raw/calibration/corrected phase deg: "); output_.print(r.rawPhaseDeg, 6);
  output_.print(" / "); output_.print(r.calibrationPhaseDeg, 6);
  output_.print(" / "); output_.println(r.correctedPhaseDeg, 6);
  output_.print("Apparent circuit impedance ohm: ");
  if (r.valid) output_.println(r.apparentImpedanceOhms, 6); else output_.println("unavailable");
  output_.print("Elapsed ms: "); output_.println(r.elapsedMs);
  output_.print("AD5934 final state/standby verified: ");
  output_.print(ad5934FinalName(result.ad5934FinalState));
  output_.print(" / "); output_.println(result.standbyVerified ? "yes" : "no");
  output_.println("Digital mask readback is not analog-route verification. Saturation not reliably detectable.");
  output_.println("EC acquisition applies no R2 subtraction or conductivity conversion. Phase is instrument-corrected only.");
}

bool CN0349Manager::processEc(char** args, size_t count, bool busReady, bool busBusy) {
  if (count == 3 && strcmp(args[2], "config") == 0) {
    const auto& p = impedance_.profile();
    output_.print("EC route/mask/RFB: "); output_.print(ecName(ecRange_));
    output_.print(" / 0x"); output_.print(ecMask(ecRange_), HEX);
    output_.print(" / "); output_.println(ecRfb(ecRange_), 0);
    output_.print("Requested Hz/MCLK/Range/PGA/settling: "); output_.print(p.frequencyHz);
    output_.print(" / "); output_.print(p.mclkHz);
    output_.print(" / "); output_.print(p.excitationRange);
    output_.print(" / "); output_.print(p.pgaGain);
    output_.print(" / "); output_.println(p.settlingCycles);
    output_.print("Calibration selected: "); output_.print(presetName(ecCalibration_));
    MuxApp::CalibrationInfo info{};
    const bool matching = impedance_.presetInfo(toMuxPreset(ecCalibration_), info) &&
        info.rfbOhms == ecRfb(ecRange_);
    output_.println(matching ? " (matching RFB)" : " (INCOMPATIBLE RFB; select matching calibration)");
  } else if (count == 4 && strcmp(args[2], "range") == 0 && strcmp(args[3], "list") == 0) {
    output_.println("HIGH: MEASURE_EC_HIGH 0x81 RFB=100 ohm; LOW: MEASURE_EC_LOW 0x82 RFB=1000 ohm.");
    output_.println("HIGH/LOW designate RFB, not guaranteed conductivity class.");
  } else if (count == 5 && strcmp(args[2], "range") == 0 && strcmp(args[3], "set") == 0) {
    CN0349ImpedanceError error = CN0349ImpedanceError::InvalidData;
    if (strcmp(args[4], "high") == 0) error = selectEcRange(CN0349EcRange::High);
    else if (strcmp(args[4], "low") == 0) error = selectEcRange(CN0349EcRange::Low);
    ecRequestError_ = error;
    output_.print("EC range selection: "); output_.println(impedanceErrorName(error));
  } else if (count == 4 && strcmp(args[2], "cal") == 0 && strcmp(args[3], "list") == 0) {
    for (uint8_t i = 0; i < 4; ++i) {
      const auto preset = static_cast<CN0349Preset>(i);
      MuxApp::CalibrationInfo info{};
      if (!impedance_.presetInfo(toMuxPreset(preset), info)) continue;
      CN0349Calibration c{};
      output_.print(presetName(preset)); output_.print(": ");
      if (info.rfbOhms != ecRfb(ecRange_)) output_.println("INCOMPATIBLE RFB");
      else if (!getCalibration(preset, c)) output_.println("UNAVAILABLE; run cn0349 cal run all");
      else {
        output_.print(calibrationCompatibleForRfb(c, ecRfb(ecRange_)) ? "COMPATIBLE" : "INCOMPATIBLE CONFIG");
        output_.print("; ID="); output_.print(c.calibrationId);
        output_.print("; GF="); output_.println(c.gainFactorPerOhm, 12);
      }
    }
  } else if (count == 5 && strcmp(args[2], "cal") == 0 && strcmp(args[3], "set") == 0) {
    CN0349Preset preset{};
    ecRequestError_ = parsePreset(args[4], preset) ? selectEcCalibration(preset) :
        CN0349ImpedanceError::InvalidCalibration;
    output_.print("EC calibration selection: "); output_.println(impedanceErrorName(ecRequestError_));
  } else if (count == 3 && strcmp(args[2], "start") == 0) {
    if (isBusy() || busBusy) ecRequestError_ = CN0349ImpedanceError::Busy;
    else if (!busReady) ecRequestError_ = CN0349ImpedanceError::NotReady;
    else startEc();
    output_.print("EC request: "); output_.println(impedanceErrorName(ecRequestError_));
  } else if (count == 3 && strcmp(args[2], "stop") == 0) {
    output_.println(cancelEc() ? "EC cancellation requested; waiting for standby." : "No active EC measurement.");
  } else if (count == 3 && strcmp(args[2], "status") == 0) {
    output_.print("EC state/error: "); output_.print(impedanceStateName(ecResult_.acquisition.state));
    output_.print(" / "); output_.println(impedanceErrorName(ecResult_.acquisition.error));
    output_.print("EC last request error: "); output_.println(impedanceErrorName(ecRequestError_));
    output_.print("EC busy: "); output_.println(isEcRunning() ? "yes" : "no");
  } else if (count == 3 && strcmp(args[2], "result") == 0) {
    printEcResult(ecResult_);
    if (!ecResult_.acquisition.valid && lastValidEcResult_.acquisition.valid) {
      output_.println("Previous valid EC result (separate acquisition):");
      printEcResult(lastValidEcResult_);
    }
  } else {
    output_.println("cn0349 ec help|config|range list|range set HIGH|LOW|cal list|cal set <PRESET>|start|status|result|stop");
    output_.println("EC HIGH/LOW is RFB; 2000 Hz; manual calibration choice; no conductivity conversion or R2 offset.");
  }
  return true;
}
