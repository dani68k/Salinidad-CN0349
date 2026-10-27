#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "../impedance/ImpedanceApp.h"
#include "../temperature/Pt100.h"
#include "../conductivity/Conductivity.h"

class Terminal;

enum class CN0349Preset : uint8_t { CalHigh100R, CalHigh1K, CalLow1K, CalLow10K };
enum class CN0349State : uint8_t { NotInitialized, Idle, RunningSingle, RunningSweep, RunningImpedance, RunningEc, Error };
enum class CN0349Operation : uint8_t { None, Single, Sweep, Impedance, Ec };
enum class CN0349Outcome : uint8_t { None, Complete, Cancelled, Failed };
enum class CN0349CalibrationState : uint8_t { Idle, Running, Complete, Failed, Cancelled };
enum class CN0349CalibrationError : uint8_t { None, Busy, InvalidArgument, NotReady, AcquisitionFailed, Cancelled };
static constexpr uint8_t CN0349_CALIBRATION_SAMPLES = 5;

struct CN0349SingleConfig {
  CN0349Preset preset = CN0349Preset::CalLow1K;
  uint32_t frequencyHz = 2000;
  uint16_t settlingCycles = 10;
};

struct CN0349SweepConfig {
  CN0349Preset preset = CN0349Preset::CalLow1K;
  uint32_t startHz = 1000;
  uint32_t stepHz = 250;
  uint16_t increments = 8;
  uint16_t settlingCycles = 10;
};

struct CN0349RawResult {
  CN0349Preset preset = CN0349Preset::CalLow1K;
  int16_t real = 0;
  int16_t imag = 0;
  float magnitude = 0;
  float phaseDeg = 0;
  float requestedFrequencyHz = 0;
  float effectiveFrequencyHz = 0;
  uint32_t elapsedMs = 0;
  bool valid = false;
};

struct CN0349Calibration {
  uint8_t presetMask = 0;
  float frequencyHz = 0;
  float effectiveFrequencyHz = 0;
  float mclkHz = 0;
  float rfbOhms = 0;
  float rcalOhms = 0;
  uint8_t excitationRange = 0;
  uint8_t pgaGain = 0;
  uint16_t settlingCycles = 0;
  uint8_t sampleCount = 0;
  int16_t real[CN0349_CALIBRATION_SAMPLES]{};
  int16_t imag[CN0349_CALIBRATION_SAMPLES]{};
  float sampleMagnitude[CN0349_CALIBRATION_SAMPLES]{};
  float samplePhaseDeg[CN0349_CALIBRATION_SAMPLES]{};
  int16_t representativeReal = 0;
  int16_t representativeImag = 0;
  float meanMagnitude = 0;
  float magnitudeStdDev = 0;
  float relativeVariationPct = 0;
  float meanPhaseDeg = 0;
  float gainFactorPerOhm = 0;
  uint32_t timestampMs = 0;
  uint32_t durationMs = 0;
  uint32_t calibrationId = 0;
  bool valid = false;
  bool compatibleWithCurrentConfig = false;
};

enum class CN0349ImpedanceState : uint8_t { Idle, Running, Complete, Failed, Cancelled };
enum class CN0349ImpedanceError : uint8_t {
  None, Busy, NotReady, InvalidFrequency, InvalidCalibration, IncompatibleCalibration,
  AcquisitionFailed, InvalidData, Cancelled
};
struct CN0349ImpedanceResult {
  bool valid = false;
  uint8_t measurementMask = 0x41;
  uint8_t calibrationMask = 0;
  CN0349Preset calibrationPreset = CN0349Preset::CalHigh100R;
  uint32_t calibrationId = 0;
  float frequencyHz = 2000;
  float effectiveFrequencyHz = 0;
  float calibrationEffectiveFrequencyHz = 0;
  uint32_t mclkHz = 0;
  uint8_t excitationRange = 0;
  uint8_t pgaGain = 0;
  uint16_t settlingCycles = 0;
  int16_t real = 0;
  int16_t imag = 0;
  float magnitude = 0;
  float rawPhaseDeg = 0;
  float calibrationPhaseDeg = 0;
  float correctedPhaseDeg = 0;
  float gainFactor = 0;
  float apparentImpedanceOhms = 0;
  uint32_t elapsedMs = 0;
  CN0349ImpedanceState state = CN0349ImpedanceState::Idle;
  CN0349ImpedanceError error = CN0349ImpedanceError::None;
  ImpedanceDiagnostics::ErrorCode hardwareError = ImpedanceDiagnostics::ErrorCode::None;
};

enum class CN0349EcRange : uint8_t { High, Low };
struct CN0349EcResult {
  CN0349EcRange range = CN0349EcRange::High;
  float rfbOhms = 100;
  CN0349ImpedanceResult acquisition{};
  bool standbyVerified = false;
  ImpedanceApp::MeasurementState ad5934FinalState = ImpedanceApp::MeasurementState::Idle;
  // No reliable saturation flag is available from the current DFT/status data.
  bool saturationKnown = false;
};

struct CN0349ConductivityResult {
  CN0349EcResult ec{};
  Conductivity::Result conversion{};
  bool valid = false;
};

// Value snapshot: acquisition provenance and conversion travel together.
struct CN0349TemperatureResult {
  CN0349ImpedanceResult acquisition{};
  Pt100::Result conversion{};
  bool valid = false;
  // Reserved policy boundary: this version always uses uncorrected apparent Z.
  bool cableCompensationApplied = false;
};

inline CN0349TemperatureResult convertPt100Acquisition(const CN0349ImpedanceResult& acquisition) {
  CN0349TemperatureResult r;
  r.acquisition = acquisition;
  if (acquisition.valid && acquisition.state == CN0349ImpedanceState::Complete &&
      acquisition.error == CN0349ImpedanceError::None &&
      acquisition.hardwareError == ImpedanceDiagnostics::ErrorCode::None)
    r.conversion = Pt100::convert(acquisition.apparentImpedanceOhms);
  r.valid = r.conversion.valid;
  return r;
}

struct CN0349SweepPoint {
  uint16_t index = 0;
  float requestedFrequencyHz = 0;
  float effectiveFrequencyHz = 0;
  int16_t real = 0;
  int16_t imag = 0;
  float magnitude = 0;
  float phaseDeg = 0;
  bool valid = false;
};

struct CN0349SweepResult {
  CN0349SweepConfig config{};
  uint16_t pointsValid = 0;
  uint16_t pointsExpected = 0;
  uint32_t elapsedMs = 0;
  CN0349Outcome outcome = CN0349Outcome::None;
  const CN0349SweepPoint* points = nullptr;
};

struct CN0349Status {
  CN0349State state = CN0349State::NotInitialized;
  CN0349Operation activeOperation = CN0349Operation::None;
  CN0349Operation lastOperation = CN0349Operation::None;
  CN0349Outcome lastOutcome = CN0349Outcome::None;
  bool busy = false;
  bool hardwareReady = false;
  bool ad5934Communication = false;
  bool adg715Communication = false;
  bool configurationVerified = false;
  bool diagnosticComplete = false;
  bool requestedPresetValid = false;
  CN0349Preset requestedPreset = CN0349Preset::CalLow1K;
  bool appliedPresetValid = false;
  CN0349Preset appliedPreset = CN0349Preset::CalLow1K;
  bool physicalMaskKnown = false;
  uint8_t physicalMask = 0;
  uint16_t pointsValid = 0;
  uint16_t pointsExpected = 0;
  uint32_t elapsedMs = 0;
  ImpedanceDiagnostics::ErrorCode activeError = ImpedanceDiagnostics::ErrorCode::None;
  ImpedanceDiagnostics::ErrorCode lastError = ImpedanceDiagnostics::ErrorCode::None;
};

struct CN0349Diagnostics {
  ImpedanceApp::ImpedanceStatus subsystem{};
  bool cached = true;
  bool appliedPresetValid = false;
  CN0349Preset appliedPreset = CN0349Preset::CalLow1K;
};

class CN0349Manager {
 public:
  static constexpr uint16_t MAX_SWEEP_POINTS = ImpedanceApp::MAX_SWEEP_POINTS;
  explicit CN0349Manager(ImpedanceApp& impedance, Stream& output);
  bool begin(Terminal& terminal, bool busReady);
  void update();

  bool isReady() const;
  bool isBusy() const;
  bool setPreset(CN0349Preset preset);
  CN0349Preset getPreset() const;
  CN0349SingleConfig getSingleConfig() const;
  bool configureSingle(const CN0349SingleConfig& config);
  bool startSingle();
  bool configureSweep(const CN0349SweepConfig& config);
  CN0349SweepConfig getSweepConfig() const;
  bool startSweep();
  bool cancel();
  CN0349Status getStatus() const;
  bool getSingleResult(CN0349RawResult& result) const;
  bool getSweepResult(CN0349SweepResult& result) const;
  CN0349Diagnostics getDiagnostics() const;
  bool runDiagnostics();
  bool recover();
  bool prepareMeasurements();
  CN0349CalibrationError startCalibrationAll(uint32_t frequencyHz = 2000);
  CN0349CalibrationError startCalibration(CN0349Preset preset, uint32_t frequencyHz = 2000);
  bool isCalibrationRunning() const;
  bool getCalibration(CN0349Preset preset, CN0349Calibration& result) const;
  bool cancelCalibration();
  CN0349CalibrationState calibrationState() const { return calibrationState_; }
  CN0349CalibrationError calibrationError() const { return calibrationError_; }
  bool clearCalibrations();
  CN0349ImpedanceError selectImpedanceCalibration(CN0349Preset preset);
  CN0349Preset impedanceCalibration() const { return impedanceCalibration_; }
  CN0349ImpedanceError startImpedance(bool quiet = false);
  bool cancelImpedance();
  bool isImpedanceRunning() const { return impedanceResult_.state == CN0349ImpedanceState::Running; }
  CN0349ImpedanceResult getImpedanceResult() const { return impedanceResult_; }
  bool getLastValidImpedanceResult(CN0349ImpedanceResult& result) const;
  CN0349ImpedanceError impedanceRequestError() const { return impedanceRequestError_; }
  CN0349ImpedanceError startTemperature(bool quiet = false);
  CN0349ImpedanceError startContinuousTemperature();
  bool isContinuousTemperature() const { return temperatureContinuous_; }
  bool cancelTemperature();
  CN0349TemperatureResult getTemperatureResult() const { return temperatureResult_; }
  CN0349ImpedanceError temperatureRequestError() const { return temperatureRequestError_; }
  CN0349ImpedanceError selectEcRange(CN0349EcRange range);
  CN0349EcRange ecRange() const { return ecRange_; }
  CN0349ImpedanceError selectEcCalibration(CN0349Preset preset);
  CN0349Preset ecCalibration() const { return ecCalibration_; }
  CN0349ImpedanceError startEc();
  bool cancelEc();
  bool isEcRunning() const { return ecResult_.acquisition.state == CN0349ImpedanceState::Running; }
  CN0349EcResult getEcResult() const { return ecResult_; }
  bool getLastValidEcResult(CN0349EcResult& result) const;
  CN0349ImpedanceError ecRequestError() const { return ecRequestError_; }
  CN0349ImpedanceError startConductivity();
  bool cancelConductivity();
  CN0349ConductivityResult getConductivityResult() const { return conductivityResult_; }
  Conductivity::Model conductivityModel() const { return conductivityModel_; }
  bool selectConductivityModel(Conductivity::Model model);
  CN0349ImpedanceError conductivityRequestError() const { return conductivityRequestError_; }


 private:
  bool calibrationCompatibleForRfb(const CN0349Calibration& calibration, float rfbOhms) const;
  bool processEc(char** args, size_t count, bool busReady, bool busBusy);
  bool processConductivity(char** args, size_t count, bool busReady, bool busBusy);
  void printConductivityResult(const CN0349ConductivityResult& result);
  void finishConductivity();
  void advanceEc();
  void printEcResult(const CN0349EcResult& result);
  CN0349EcRange ecRange_ = CN0349EcRange::High;
  CN0349Preset ecCalibration_ = CN0349Preset::CalHigh100R;
  CN0349Calibration ecCalibrationSnapshot_{};
  CN0349EcResult ecResult_{};
  CN0349EcResult lastValidEcResult_{};
  CN0349ImpedanceError ecRequestError_ = CN0349ImpedanceError::None;
  bool ecCancelRequested_ = false;
  uint32_t ecStartedMs_ = 0;
  Conductivity::Model conductivityModel_ = Conductivity::Model::Raw;
  CN0349ConductivityResult conductivityResult_{};
  CN0349ImpedanceError conductivityRequestError_ = CN0349ImpedanceError::None;
  bool conductivityRunning_ = false;
  bool preparationPending_ = false;
  void advanceTemperature();
  void scheduleTemperature();
  void printTemperatureSample();
  bool processTemperature(char** args, size_t count, bool busReady, bool busBusy);
  void printTemperatureStatus();
  CN0349TemperatureResult temperatureResult_{};
  CN0349ImpedanceError temperatureRequestError_ = CN0349ImpedanceError::None;
  bool temperatureRunning_ = false;
  bool temperatureContinuous_ = false;
  uint32_t temperatureLastStartMs_ = 0;
  static bool dispatch(void* context, char* line, bool busReady, bool busBusy);
  static bool busy(void* context);
  bool process(char* line, bool busReady, bool busBusy);
  bool parsePreset(const char* name, CN0349Preset& preset) const;
  static MuxApp::CalibrationPreset toMuxPreset(CN0349Preset preset);
  static const char* presetName(CN0349Preset preset);
  static const char* stateName(CN0349State state);
  static const char* operationName(CN0349Operation operation);
  static const char* outcomeName(CN0349Outcome outcome);
  static const char* calibrationErrorName(CN0349CalibrationError error);
  void printConfig();
  void printStatus();
  void printResult();
  void synchronizeOutcome();
  bool startCalibrationSample();
  void advanceCalibration();
  void finishCalibrationPreset();
  void printCalibration(const CN0349Calibration& calibration);
  void printCalibrationStatus();
  void printCalibrationList();
  void printCalibrationResult();
  void printCalibrationHelp();
  bool calibrationCompatible(const CN0349Calibration& calibration) const;

  bool impedanceCalibrationCompatible(const CN0349Calibration& calibration) const;
  void advanceImpedance();
  bool processImpedance(char** args, size_t count, bool busReady, bool busBusy);
  void printImpedanceConfig();
  void printImpedanceStatus();
  void printImpedanceResult(const CN0349ImpedanceResult& result);
  static const char* impedanceErrorName(CN0349ImpedanceError error);
  static const char* impedanceStateName(CN0349ImpedanceState state);
  CN0349Preset impedanceCalibration_ = CN0349Preset::CalHigh100R;
  CN0349Calibration impedanceCalibrationSnapshot_{};
  CN0349ImpedanceResult impedanceResult_{};
  CN0349ImpedanceResult lastValidImpedanceResult_{};
  CN0349ImpedanceError impedanceRequestError_ = CN0349ImpedanceError::None;
  bool impedanceCancelRequested_ = false;
  bool impedanceQuiet_ = false;
  uint32_t impedanceStartedMs_ = 0;

  ImpedanceApp& impedance_;
  Stream& output_;
  bool initialized_ = false;
  bool busReady_ = false;
  bool observedBusy_ = false;
  bool cancelledByManager_ = false;
  bool resultOperationValid_ = false;
  CN0349State state_ = CN0349State::NotInitialized;
  CN0349Operation activeOperation_ = CN0349Operation::None;
  CN0349Operation lastOperation_ = CN0349Operation::None;
  CN0349Outcome lastOutcome_ = CN0349Outcome::None;
  mutable CN0349SweepPoint sweepSnapshot_[MAX_SWEEP_POINTS]{};
  CN0349Calibration calibrations_[4]{};
  CN0349Calibration candidateCalibration_{};
  CN0349Preset calibrationSequence_[4]{};
  CN0349Preset activeCalibrationPreset_ = CN0349Preset::CalLow1K;
  CN0349Preset lastCalibrationPreset_ = CN0349Preset::CalLow1K;
  CN0349CalibrationState calibrationState_ = CN0349CalibrationState::Idle;
  CN0349CalibrationError calibrationError_ = CN0349CalibrationError::None;
  bool calibrationAll_ = false;
  bool calibrationSampleInFlight_ = false;
  bool calibrationCancelRequested_ = false;
  uint8_t calibrationSequenceIndex_ = 0;
  uint8_t calibrationSampleIndex_ = 0;
  uint32_t calibrationStartedMs_ = 0;
  uint32_t activePresetStartedMs_ = 0;
  uint32_t nextCalibrationId_ = 1;
  float calibrationSumMagnitude_ = 0;
  float calibrationSumPhaseSin_ = 0;
  float calibrationSumPhaseCos_ = 0;
  float calibrationMeanEffectiveHz_ = 0;
};
