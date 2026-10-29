#include "CN0349Manager.h"
#include "ImpedanceMath.h"
#include "config.h"
#include <math.h>
#include <Preferences.h>
#include <string.h>
namespace {
bool selected(MeasurementSensors s, MeasurementSensors bit) {
  return (static_cast<uint8_t>(s) & static_cast<uint8_t>(bit)) != 0;
}
MuxApp::CalibrationPreset muxPreset(CN0349Preset p) { return static_cast<MuxApp::CalibrationPreset>(p); }
}
InstrumentError CN0349Manager::begin(bool busReady) {
  if (initialized_) return InstrumentError::Busy;
  loadSetup();
  initialized_ = true; busReady_ = busReady;
  if (!acquisition_.requestRecovery(busReady_)) {
    state_ = InstrumentState::Error; return lastError_ = InstrumentError::Hardware;
  }
  beginCalibration(); return InstrumentError::None;
}
bool CN0349Manager::isBusy() const {
  return continuous_ || cycleActive_ || acquisition_.isBusy() || calibrationPending_ ||
      state_ == InstrumentState::Calibrating || state_ == InstrumentState::Stopping;
}
InstrumentStatus CN0349Manager::getStatus() const {
  InstrumentStatus s;
  s.state = state_; s.continuous = continuous_;
  s.hardwareReady = acquisition_.statusSnapshot().hardwareReady;
  for (const auto& c : calibrations_) if (compatible(c, c.rfbOhms)) ++s.validCalibrations;
  s.adHighValid = compatible(adCalibrations_[0], CN0349EcRange::High);
  s.adLowValid = compatible(adCalibrations_[1], CN0349EcRange::Low);
  s.calibrationRunId = calibrationRunId_; s.intervalExceededCount = intervalExceededCount_;
  s.publishedCycles = publishedCycles_; s.overwrittenResults = overwrittenResults_; s.lastError = lastError_;
  return s;
}
bool CN0349Manager::compatible(const Conductivity::AdCalibration& c, CN0349EcRange range) const {
  const auto& p = acquisition_.profile();
  const double rfb = range == CN0349EcRange::High ? 100.0 : 1000.0;
  return c.valid && c.rfbOhms == rfb && isfinite(c.gainFactorSiemensPerCode) &&
      c.gainFactorSiemensPerCode > 0 && isfinite(c.systemOffsetCode) &&
      c.frequencyHz == p.frequencyHz && c.mclkHz == p.mclkHz && c.settlingCycles == p.settlingCycles &&
      c.excitationRange == p.excitationRange && c.pgaGain == p.pgaGain;
}
bool CN0349Manager::compatible(const CN0349Calibration& c, float rfb) const {
  const auto& p = acquisition_.profile();
  return c.valid && c.state == CalibrationState::Valid && c.rfbOhms == rfb &&
      isfinite(c.gainFactorPerOhm) && c.gainFactorPerOhm > 0 &&
      isfinite(c.meanMagnitude) && c.meanMagnitude > 0 && isfinite(c.meanPhaseDeg) &&
      c.frequencyHz == p.frequencyHz && c.mclkHz == p.mclkHz &&
      c.excitationRange == p.excitationRange && c.pgaGain == p.pgaGain && c.settlingCycles == p.settlingCycles;
}
bool CN0349Manager::getCalibrationStatus(CN0349Preset preset, CN0349Calibration& result) const {
  const auto i = static_cast<uint8_t>(preset);
  if (i >= 4) return false;
  result = calibrations_[i]; result.compatibleWithCurrentConfig = compatible(result, result.rfbOhms); return true;
}
bool CN0349Manager::getAdCalibrationStatus(CN0349EcRange range, Conductivity::AdCalibration& result) const {
  if (range != CN0349EcRange::High && range != CN0349EcRange::Low) return false;
  result = adCalibrations_[range == CN0349EcRange::High ? 0 : 1];
  return true;
}
bool CN0349Manager::validSetup(const InstrumentSetup& setup) {
  const auto sensors = static_cast<uint8_t>(setup.sensors), preset = static_cast<uint8_t>(setup.ecCalibration);
  return sensors >= 1 && sensors <= 3 && setup.intervalMs > 0 && setup.intervalMs <= 0x7fffffffUL &&
      (setup.ecRange == CN0349EcRange::High || setup.ecRange == CN0349EcRange::Low) &&
      preset < 4 && (setup.ecRange == CN0349EcRange::High ? preset < 2 : preset >= 2) &&
      (setup.ecModel == Conductivity::Model::Raw || setup.ecModel == Conductivity::Model::SeriesR2Experimental ||
       setup.ecModel == Conductivity::Model::AnalogDevices) && isfinite(setup.nominalKPerCm) && setup.nominalKPerCm > 0;
}

void CN0349Manager::loadSetup() {
  Preferences preferences;
  if (!preferences.begin("cn0349", true)) return;
  uint8_t blob[20]{};
  const size_t length = preferences.getBytesLength("setup");
  const bool read = (length == 12 || length == sizeof(blob)) &&
      preferences.getBytes("setup", blob, length) == length;
  preferences.end();
  if (!read || !((blob[0] == 1 && length == 12) || (blob[0] == 2 && length == sizeof(blob))) ||
      blob[1] < 1 || blob[1] > 3 || blob[2] > 1 || blob[3] > 2) return;
  InstrumentSetup candidate = setup_;
  candidate.sensors = static_cast<MeasurementSensors>(blob[1]);
  candidate.ecRange = blob[2] ? CN0349EcRange::High : CN0349EcRange::Low;
  candidate.ecCalibration = blob[2] ? CN0349Preset::CalHigh100R : CN0349Preset::CalLow10K;
  candidate.ecModel = static_cast<Conductivity::Model>(blob[3]);
  candidate.intervalMs = static_cast<uint32_t>(blob[4]) | (static_cast<uint32_t>(blob[5]) << 8) |
      (static_cast<uint32_t>(blob[6]) << 16) | (static_cast<uint32_t>(blob[7]) << 24);
  static_assert(sizeof(float) == 4, "Setup NVS requires float32");
  memcpy(&candidate.nominalKPerCm, blob + 8, sizeof(float));
  if (validSetup(candidate)) setup_ = candidate;
}

bool CN0349Manager::saveSetup() const {
  uint8_t blob[12]{};
  blob[0] = 1;
  blob[1] = static_cast<uint8_t>(setup_.sensors);
  blob[2] = setup_.ecRange == CN0349EcRange::High ? 1 : 0;
  blob[3] = static_cast<uint8_t>(setup_.ecModel);
  for (size_t i = 0; i < 4; ++i) blob[4 + i] = static_cast<uint8_t>(setup_.intervalMs >> (8 * i));
  memcpy(blob + 8, &setup_.nominalKPerCm, sizeof(float));
  Preferences preferences;
  if (!preferences.begin("cn0349", false)) return false;
  const bool written = preferences.putBytes("setup", blob, sizeof(blob)) == sizeof(blob);
  preferences.end();
  return written;
}

InstrumentError CN0349Manager::setSetup(const InstrumentSetup& setup) {
  if (isBusy()) return InstrumentError::Busy;
  if (!validSetup(setup)) return InstrumentError::InvalidArgument;
  const bool changed = setup.sensors != setup_.sensors || setup.intervalMs != setup_.intervalMs ||
      setup.ecRange != setup_.ecRange || setup.ecModel != setup_.ecModel || setup.nominalKPerCm != setup_.nominalKPerCm;
  setup_ = setup;
  if (changed) setupPersistenceFailed_ = !saveSetup();
  return setupPersistenceFailed_ ? InstrumentError::Persistence : InstrumentError::None;
}
InstrumentError CN0349Manager::setMeasurementSensors(MeasurementSensors sensors) {
  auto setup = setup_; setup.sensors = sensors; return setSetup(setup);
}
InstrumentError CN0349Manager::runCalibration() {
  if (!initialized_) return InstrumentError::Hardware;
  if (state_ == InstrumentState::Calibrating || calibrationPending_ || (isBusy() && !continuous_)) return InstrumentError::Busy;
  if (continuous_) {
    stopMeasurement(); calibrationPending_ = true; return InstrumentError::None;
  }
  if (!acquisition_.requestRecovery(busReady_)) {
    for (auto& c : calibrations_) { c.valid = false; c.state = CalibrationState::Invalid; c.error = InstrumentError::Hardware; }
    ++calibrationRunId_;  // Publish failed asynchronous requests after stop, too.
    state_ = InstrumentState::Error; return lastError_ = InstrumentError::Hardware;
  }
  beginCalibration(); return InstrumentError::None;
}
void CN0349Manager::beginCalibration() {
  for (uint8_t i = 0; i < 4; ++i) {
    calibrations_[i] = CN0349Calibration{}; calibrations_[i].preset = static_cast<CN0349Preset>(i);
  }
  adCalibrations_[0] = Conductivity::AdCalibration{};
  adCalibrations_[1] = Conductivity::AdCalibration{};
  state_ = InstrumentState::Calibrating; calibrationSlot_ = 0; calibrationSampleStarted_ = false;
  lastError_ = InstrumentError::None; startCalibrationSlot();
}
void CN0349Manager::buildAdCalibrations() {
  struct Pair { uint8_t low; uint8_t high; double yLow; double yHigh; double rfb; };
  const Pair pairs[2] = {{1, 0, 0.001, 0.01, 100.0}, {3, 2, 0.0001, 0.001, 1000.0}};
  for (uint8_t i = 0; i < 2; ++i) {
    const auto& low = calibrations_[pairs[i].low]; const auto& high = calibrations_[pairs[i].high];
    if (!low.valid || !high.valid || low.error != InstrumentError::None || high.error != InstrumentError::None) {
      adCalibrations_[i] = Conductivity::AdCalibration{};
      adCalibrations_[i].error = Conductivity::AdCalibrationError::InvalidPoint;
      continue;
    }
    if (low.rfbOhms != high.rfbOhms || low.frequencyHz != high.frequencyHz ||
        !CN0349Math::sameEffectiveFrequency(low.effectiveFrequencyHz, high.effectiveFrequencyHz) ||
        low.mclkHz != high.mclkHz || low.excitationRange != high.excitationRange ||
        low.pgaGain != high.pgaGain || low.settlingCycles != high.settlingCycles) {
      adCalibrations_[i] = Conductivity::AdCalibration{};
      adCalibrations_[i].error = Conductivity::AdCalibrationError::ConfigurationMismatch;
      continue;
    }
    const uint32_t id = (low.calibrationId << 16) ^ high.calibrationId;
    adCalibrations_[i] = Conductivity::calibrateAnalogDevices(low.meanMagnitude, high.meanMagnitude,
        pairs[i].yLow, pairs[i].yHigh, pairs[i].rfb, low.frequencyHz,
        low.excitationRange, low.pgaGain, id);
    adCalibrations_[i].effectiveFrequencyHz = (low.effectiveFrequencyHz + high.effectiveFrequencyHz) / 2.0;
    adCalibrations_[i].mclkHz = low.mclkHz;
    adCalibrations_[i].settlingCycles = low.settlingCycles;
  }
}
void CN0349Manager::startCalibrationSlot() {
  auto& c = calibrations_[calibrationSlot_]; MuxApp::CalibrationInfo info{};
  acquisition_.presetInfo(muxPreset(c.preset), info);
  const auto& p = acquisition_.profile();
  c.presetMask = info.mask; c.rfbOhms = info.rfbOhms; c.rcalOhms = info.rcalOhms;
  c.frequencyHz = 2000; c.mclkHz = static_cast<float>(p.mclkHz); c.excitationRange = p.excitationRange;
  c.pgaGain = p.pgaGain; c.settlingCycles = 10; c.calibrationId = nextCalibrationId_++;
  c.state = CalibrationState::Running; c.error = InstrumentError::None;
  calibrationSlotStartedMs_ = millis(); calibrationSampleStarted_ = false;
}
InstrumentError CN0349Manager::acquisitionError() const {
  const auto s = acquisition_.statusSnapshot(); using E = ImpedanceDiagnostics::ErrorCode;
  if (s.activeError == E::RecoveryFailed) return InstrumentError::Hardware;
  if (s.lastError == E::DftTimeout || s.lastError == E::I2cTimeout) return InstrumentError::Timeout;
  return InstrumentError::Hardware;
}
void CN0349Manager::updateCalibration() {
  if (acquisition_.isBusy()) return;
  auto& c = calibrations_[calibrationSlot_];
  if (!calibrationSampleStarted_) {
    if (!acquisition_.statusSnapshot().hardwareReady && !acquisition_.requestRecovery(busReady_)) {
      completeCalibrationSlot(InstrumentError::Hardware); return;
    }
    const bool configured = acquisition_.configureSingle(muxPreset(c.preset), 2000, 10);
    calibrationSampleStarted_ = true;
    if (!configured || !acquisition_.requestSingle(busReady_)) c.error = acquisitionError();
    return;
  }
  if (c.error != InstrumentError::None) { completeCalibrationSlot(c.error); return; }
  ImpedanceApp::Result raw{};
  if (!acquisition_.singleResult(raw)) { completeCalibrationSlot(acquisitionError()); return; }
  if (!raw.valid || !isfinite(raw.magnitude) || raw.magnitude <= 0 || !isfinite(raw.phaseDegrees) ||
      raw.calibration.mask != c.presetMask || raw.frequencyHz != 2000 ||
      (c.sampleCount && !CN0349Math::sameEffectiveFrequency(c.effectiveFrequencyHz, static_cast<float>(raw.effectiveFrequencyHz)))) {
    completeCalibrationSlot(InstrumentError::MeasurementFailed); return;
  }
  const auto i = c.sampleCount++;
  c.real[i] = raw.real; c.imag[i] = raw.imaginary; c.sampleMagnitude[i] = static_cast<float>(raw.magnitude); c.samplePhaseDeg[i] = static_cast<float>(raw.phaseDegrees);
  c.effectiveFrequencyHz = static_cast<float>(raw.effectiveFrequencyHz); calibrationSampleStarted_ = false;
  if (c.sampleCount == CN0349_CALIBRATION_SAMPLES) completeCalibrationSlot(InstrumentError::None);
}
void CN0349Manager::completeCalibrationSlot(InstrumentError error) {
  auto& c = calibrations_[calibrationSlot_];
  c.timestampMs = millis(); c.durationMs = millis() - calibrationSlotStartedMs_;
  c.hardwareError = error == InstrumentError::None || error == InstrumentError::MeasurementFailed ?
      ImpedanceDiagnostics::ErrorCode::None : acquisition_.statusSnapshot().lastError;
  if (error == InstrumentError::None) {
    float sum = 0, sinSum = 0, cosSum = 0, variance = 0, frequencySum = 0;
    int32_t realSum = 0, imagSum = 0;
    for (uint8_t i = 0; i < CN0349_CALIBRATION_SAMPLES; ++i) {
      sum += c.sampleMagnitude[i]; sinSum += sinf(c.samplePhaseDeg[i] * 0.017453292519943295f);
      cosSum += cosf(c.samplePhaseDeg[i] * 0.017453292519943295f);
      realSum += c.real[i]; imagSum += c.imag[i]; frequencySum += c.effectiveFrequencyHz;
    }
    c.meanMagnitude = sum / CN0349_CALIBRATION_SAMPLES;
    for (const auto m : c.sampleMagnitude) { const float d = m - c.meanMagnitude; variance += d*d; }
    c.magnitudeStdDev = sqrtf(variance / (CN0349_CALIBRATION_SAMPLES - 1));
    c.relativeVariationPct = 100 * c.magnitudeStdDev / c.meanMagnitude;
    c.meanPhaseDeg = atan2f(sinSum, cosSum) * 57.29577951308232f;
    c.gainFactorPerOhm = 1.0f / (c.rcalOhms * c.meanMagnitude);
    c.effectiveFrequencyHz = frequencySum / CN0349_CALIBRATION_SAMPLES;
    c.representativeReal = realSum / CN0349_CALIBRATION_SAMPLES; c.representativeImag = imagSum / CN0349_CALIBRATION_SAMPLES;
    if (!isfinite(c.gainFactorPerOhm) || c.gainFactorPerOhm <= 0 || !isfinite(c.meanMagnitude) || c.meanMagnitude <= 0)
      error = InstrumentError::MeasurementFailed;
  }
  c.error = error; c.valid = error == InstrumentError::None;
  c.state = c.valid ? CalibrationState::Valid : CalibrationState::Invalid;
  c.compatibleWithCurrentConfig = compatible(c, c.rfbOhms);
  if (++calibrationSlot_ < 4) { startCalibrationSlot(); return; }
  buildAdCalibrations();
  ++calibrationRunId_;
  state_ = acquisition_.statusSnapshot().hardwareReady ? InstrumentState::Idle : InstrumentState::Error;
  lastError_ = getStatus().validCalibrations == 4 ? InstrumentError::None : InstrumentError::NotCalibrated;
}
InstrumentError CN0349Manager::preflight() const {
  if (isBusy()) return InstrumentError::Busy;
  if (!initialized_ || !acquisition_.statusSnapshot().hardwareReady) return InstrumentError::Hardware;
  if (selected(setup_.sensors, MeasurementSensors::Pt100) && !compatible(calibrations_[0], 100)) return InstrumentError::NotCalibrated;
  const float rfb = setup_.ecRange == CN0349EcRange::High ? 100.0f : 1000.0f;
  if (selected(setup_.sensors, MeasurementSensors::Ec) &&
      !compatible(calibrations_[static_cast<uint8_t>(setup_.ecCalibration)], rfb)) return InstrumentError::NotCalibrated;
  if (selected(setup_.sensors, MeasurementSensors::Ec) && setup_.ecModel == Conductivity::Model::AnalogDevices &&
      !compatible(adCalibrations_[setup_.ecRange == CN0349EcRange::High ? 0 : 1], setup_.ecRange))
    return InstrumentError::NotCalibrated;
  return InstrumentError::None;
}
InstrumentError CN0349Manager::startSingleMeasurement() {
  const auto e = preflight(); if (e != InstrumentError::None) return e;
  beginCycle(); return InstrumentError::None;
}
InstrumentError CN0349Manager::startContinuousMeasurement() {
  const auto e = preflight(); if (e != InstrumentError::None) return e;
  continuous_ = true; beginCycle(); return InstrumentError::None;
}
void CN0349Manager::beginCycle() {
  cycle_ = MeasurementCycle{}; cycle_.cycleId = nextCycleId_++; cycle_.setup = setup_;
  cycle_.startedMs = lastCycleStartedMs_ = millis(); cycle_.ec.sensor = MeasurementSensors::Ec;
  cycleActive_ = true; state_ = InstrumentState::Measuring;
  startSensor(selected(setup_.sensors, MeasurementSensors::Pt100) ? MeasurementSensors::Pt100 : MeasurementSensors::Ec);
}
void CN0349Manager::startSensor(MeasurementSensors sensor) {
  activeSensor_ = sensor; sensorStarted_ = false; sensorStartedMs_ = millis();
  auto& r = sensor == MeasurementSensors::Pt100 ? cycle_.pt100 : cycle_.ec; auto& a = r.acquisition;
  const auto preset = sensor == MeasurementSensors::Pt100 ? CN0349Preset::CalHigh100R : cycle_.setup.ecCalibration;
  const auto& c = calibrations_[static_cast<uint8_t>(preset)]; const auto& p = acquisition_.profile();
  a.measurementMask = sensor == MeasurementSensors::Pt100 ? 0x41 : cycle_.setup.ecRange == CN0349EcRange::High ? 0x81 : 0x82;
  a.calibrationPreset = preset; a.calibrationMask = c.presetMask; a.calibrationId = c.calibrationId; a.rfbOhms = c.rfbOhms; a.rcalOhms = c.rcalOhms;
  a.gainFactor = c.gainFactorPerOhm; a.calibrationPhaseDeg = c.meanPhaseDeg; a.calibrationEffectiveFrequencyHz = c.effectiveFrequencyHz;
  a.frequencyHz = static_cast<float>(p.frequencyHz); a.mclkHz = p.mclkHz; a.excitationRange = p.excitationRange;
  a.pgaGain = p.pgaGain; a.settlingCycles = p.settlingCycles; a.state = CN0349ImpedanceState::Running;
  r.error = InstrumentError::None;
  if (!acquisition_.statusSnapshot().hardwareReady && !acquisition_.requestRecovery(busReady_)) { r.error = InstrumentError::Hardware; return; }
  sensorStarted_ = sensor == MeasurementSensors::Pt100 ? acquisition_.requestPt100(busReady_) : acquisition_.requestEc(busReady_, a.measurementMask);
  if (!sensorStarted_) r.error = acquisitionError();
}
void CN0349Manager::finishSensor() {
  auto& r = activeSensor_ == MeasurementSensors::Pt100 ? cycle_.pt100 : cycle_.ec; auto& a = r.acquisition;
  a.elapsedMs = millis() - sensorStartedMs_;
  ImpedanceApp::Result raw{};
  if (r.error == InstrumentError::None && (!sensorStarted_ || !acquisition_.singleResult(raw))) r.error = acquisitionError();
  if (r.error == InstrumentError::None) {
    a.real = raw.real; a.imag = raw.imaginary; a.magnitude = static_cast<float>(raw.magnitude); a.rawPhaseDeg = static_cast<float>(raw.phaseDegrees);
    a.effectiveFrequencyHz = static_cast<float>(raw.effectiveFrequencyHz);
    const bool commonValid = raw.valid && raw.calibration.mask == a.measurementMask && raw.frequencyHz == a.frequencyHz &&
        CN0349Math::sameEffectiveFrequency(a.effectiveFrequencyHz, a.calibrationEffectiveFrequencyHz) &&
        isfinite(a.magnitude) && a.magnitude > 0;
    const bool needsLegacyImpedance = activeSensor_ == MeasurementSensors::Pt100 ||
        cycle_.setup.ecModel != Conductivity::Model::AnalogDevices;
    if (!commonValid || (needsLegacyImpedance && !CN0349Math::apparentImpedance(a.magnitude, a.gainFactor,
        a.rawPhaseDeg, a.calibrationPhaseDeg, a.apparentImpedanceOhms, a.correctedPhaseDeg))) r.error = InstrumentError::MeasurementFailed;
    else {
      a.valid = true;
      if (activeSensor_ == MeasurementSensors::Pt100) { r.temperature = Pt100::convert(a.apparentImpedanceOhms); r.valid = r.temperature.valid; }
      else if (cycle_.setup.ecModel == Conductivity::Model::AnalogDevices) {
        const auto& ad = adCalibrations_[cycle_.setup.ecRange == CN0349EcRange::High ? 0 : 1];
        r.conductivity = Conductivity::convertAnalogDevices(a.magnitude, cycle_.setup.nominalKPerCm,
            Config::EC_NOMINAL_R2_OHMS, ad, a.effectiveFrequencyHz, a.rfbOhms, a.excitationRange, a.pgaGain);
        r.valid = r.conductivity.valid;
      } else { r.conductivity = Conductivity::convert(a.apparentImpedanceOhms, cycle_.setup.nominalKPerCm,
                 cycle_.setup.ecModel, Config::EC_NOMINAL_R2_OHMS); r.valid = r.conductivity.valid; }
      if (!r.valid) r.error = InstrumentError::MeasurementFailed;
    }
  }
  a.state = a.valid ? CN0349ImpedanceState::Complete : CN0349ImpedanceState::Failed;
  a.error = a.valid ? InstrumentError::None : r.error;
  a.hardwareError = r.error == InstrumentError::Hardware || r.error == InstrumentError::Timeout ? acquisition_.statusSnapshot().lastError : ImpedanceDiagnostics::ErrorCode::None;
  if (activeSensor_ == MeasurementSensors::Pt100 && selected(cycle_.setup.sensors, MeasurementSensors::Ec)) startSensor(MeasurementSensors::Ec);
  else finishCycle();
}
void CN0349Manager::finishCycle(bool cancelled) {
  cycle_.finishedMs = millis(); cycle_.cancelled = cancelled;
  cycle_.error = cancelled ? InstrumentError::Cancelled : InstrumentError::None;
  if (cancelled) {
    SensorMeasurement* results[] = {&cycle_.pt100, &cycle_.ec};
    for (auto* r : results) {
      if (selected(cycle_.setup.sensors, r->sensor) && !r->valid) {
        r->error = InstrumentError::Cancelled;
        r->acquisition.valid = false;
        r->acquisition.error = InstrumentError::Cancelled;
        r->acquisition.state = CN0349ImpedanceState::Cancelled;
      }
    }
    auto& active = activeSensor_ == MeasurementSensors::Pt100 ? cycle_.pt100 : cycle_.ec;
    active.acquisition.elapsedMs = millis() - sensorStartedMs_;
    if (!acquisition_.statusSnapshot().hardwareReady) {
      cycle_.error = active.error = active.acquisition.error = InstrumentError::Hardware;
      active.acquisition.state = CN0349ImpedanceState::Failed;
      active.acquisition.hardwareError = acquisition_.statusSnapshot().activeError;
    }
  }
  if (!cancelled) {
    if (selected(cycle_.setup.sensors, MeasurementSensors::Pt100) && !cycle_.pt100.valid) cycle_.error = cycle_.pt100.error;
    if (selected(cycle_.setup.sensors, MeasurementSensors::Ec) && !cycle_.ec.valid) cycle_.error = cycle_.ec.error;
  }
  cycle_.valid = cycle_.error == InstrumentError::None;
  cycle_.intervalExceeded = continuous_ && (cycle_.finishedMs - cycle_.startedMs > cycle_.setup.intervalMs);
  if (cycle_.intervalExceeded) ++intervalExceededCount_;
  if (resultPending_) ++overwrittenResults_;
  result_ = cycle_; hasResult_ = resultPending_ = true; ++publishedCycles_; cycleActive_ = false;
  lastError_ = cycle_.error;
  if (cancelled || !acquisition_.statusSnapshot().hardwareReady) continuous_ = false;
  state_ = acquisition_.statusSnapshot().hardwareReady ? InstrumentState::Idle : InstrumentState::Error;
}
InstrumentError CN0349Manager::stopMeasurement() {
  if (state_ == InstrumentState::Calibrating || calibrationPending_) return InstrumentError::Busy;
  continuous_ = false;
  if (cycleActive_) {
    state_ = InstrumentState::Stopping;
    if (acquisition_.isBusy()) acquisition_.cancelOperation();
  }
  return InstrumentError::None;
}
void CN0349Manager::update() {
  if (!initialized_) return;
  acquisition_.update();
  if (state_ == InstrumentState::Stopping) {
    if (acquisition_.isBusy()) return;
    if (cycleActive_) finishCycle(true);
  }
  if (calibrationPending_ && !acquisition_.isBusy() && !cycleActive_) {
    calibrationPending_ = false; runCalibration(); return;
  }
  if (state_ == InstrumentState::Calibrating) { updateCalibration(); return; }
  if (cycleActive_) { if (!acquisition_.isBusy()) finishSensor(); return; }
  // Elapsed unsigned millis survives wrap. Rebase each actual start, no backlog.
  if (continuous_ && millis() - lastCycleStartedMs_ >= setup_.intervalMs) beginCycle();
}
bool CN0349Manager::getMeasurementResult(MeasurementCycle& result) const {
  if (!hasResult_) return false;
  result = result_; return true;
}
bool CN0349Manager::takeMeasurementResult(MeasurementCycle& result) {
  if (!resultPending_) return false;
  result = result_; resultPending_ = false; return true;
}
