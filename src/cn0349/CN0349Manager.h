#pragma once
#include "../impedance/ImpedanceApp.h"
#include "../temperature/Pt100.h"
#include "../conductivity/Conductivity.h"

enum class CN0349Preset : uint8_t { CalHigh100R, CalHigh1K, CalLow1K, CalLow10K };
enum class InstrumentError : uint8_t { None, Busy, InvalidArgument, Hardware, NotCalibrated,
  Timeout, MeasurementFailed, Cancelled, Persistence };
enum class InstrumentState : uint8_t { NotInitialized, Calibrating, Idle, Measuring, Stopping, Error };
enum class CalibrationState : uint8_t { Invalid, Running, Valid };
enum class CN0349EcRange : uint8_t { High, Low };
enum class MeasurementSensors : uint8_t { Pt100 = 1, Ec = 2, Both = 3 };
static constexpr uint8_t CN0349_CALIBRATION_SAMPLES = 5;
struct CN0349Calibration {
  CN0349Preset preset = CN0349Preset::CalHigh100R;
  CalibrationState state = CalibrationState::Invalid;
  InstrumentError error = InstrumentError::NotCalibrated;
  ImpedanceDiagnostics::ErrorCode hardwareError = ImpedanceDiagnostics::ErrorCode::None;
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
  float rfbOhms = 0, rcalOhms = 0;
  float apparentImpedanceOhms = 0;
  uint32_t elapsedMs = 0;
  CN0349ImpedanceState state = CN0349ImpedanceState::Idle;
  InstrumentError error = InstrumentError::None;
  ImpedanceDiagnostics::ErrorCode hardwareError = ImpedanceDiagnostics::ErrorCode::None;
};


struct InstrumentSetup {
  MeasurementSensors sensors = MeasurementSensors::Both;
  uint32_t intervalMs = 1000;
  CN0349EcRange ecRange = CN0349EcRange::Low;
  CN0349Preset ecCalibration = CN0349Preset::CalLow10K;
  Conductivity::Model ecModel = Conductivity::Model::Raw;
  float nominalKPerCm = 1.0f;
};
struct SensorMeasurement {
  MeasurementSensors sensor = MeasurementSensors::Pt100;
  CN0349ImpedanceResult acquisition{};
  Pt100::Result temperature{};
  Conductivity::Result conductivity{};
  InstrumentError error = InstrumentError::MeasurementFailed;
  bool valid = false;
};
struct MeasurementCycle {
  uint32_t cycleId = 0;
  uint32_t startedMs = 0, finishedMs = 0;
  InstrumentSetup setup{};
  SensorMeasurement pt100{}, ec{};
  bool valid = false;
  bool cancelled = false;
  bool intervalExceeded = false;
  InstrumentError error = InstrumentError::None;
};
struct InstrumentStatus {
  InstrumentState state = InstrumentState::NotInitialized;
  bool hardwareReady = false, continuous = false;
  uint8_t validCalibrations = 0;
  uint32_t calibrationRunId = 0;
  uint32_t intervalExceededCount = 0;
  uint32_t publishedCycles = 0;
  uint32_t overwrittenResults = 0;
  InstrumentError lastError = InstrumentError::None;
  bool adLowValid = false, adHighValid = false;
};

// Central instrument API: no Stream, Serial, parser, BLE or dynamic allocation.
class CN0349Manager {
 public:
  explicit CN0349Manager(ImpedanceApp& acquisition) : acquisition_(acquisition) {}
  InstrumentError begin(bool busReady);
  void update();
  InstrumentStatus getStatus() const;
  InstrumentSetup getSetup() const { return setup_; }
  ImpedanceApp::Profile getImpedanceProfile() const { return acquisition_.profile(); }
  InstrumentError setSetup(const InstrumentSetup& setup);
  InstrumentError setMeasurementSensors(MeasurementSensors sensors);
  InstrumentError runCalibration();
  bool getCalibrationStatus(CN0349Preset preset, CN0349Calibration& result) const;
  bool getAdCalibrationStatus(CN0349EcRange range, Conductivity::AdCalibration& result) const;
  InstrumentError startSingleMeasurement();
  InstrumentError startContinuousMeasurement();
  InstrumentError stopMeasurement();
  bool isBusy() const;
  bool getMeasurementResult(MeasurementCycle& result) const;
  // One shared consumer slot. False means no unconsumed cycle. See architecture.md.
  bool takeMeasurementResult(MeasurementCycle& result);
 private:
  static bool validSetup(const InstrumentSetup& setup);
  void loadSetup();
  bool saveSetup() const;
  bool compatible(const CN0349Calibration& c, float rfb) const;
  bool compatible(const Conductivity::AdCalibration& c, CN0349EcRange range) const;
  void buildAdCalibrations();
  InstrumentError preflight() const;
  void beginCalibration();
  void startCalibrationSlot();
  void updateCalibration();
  void completeCalibrationSlot(InstrumentError error);
  void beginCycle();
  void startSensor(MeasurementSensors sensor);
  void finishSensor();
  void finishCycle(bool cancelled = false);
  InstrumentError acquisitionError() const;
  ImpedanceApp& acquisition_;
  bool initialized_ = false, busReady_ = false;
  bool continuous_ = false, cycleActive_ = false, sensorStarted_ = false;
  bool calibrationPending_ = false;
  InstrumentState state_ = InstrumentState::NotInitialized;
  InstrumentError lastError_ = InstrumentError::None;
  InstrumentSetup setup_{};
  bool setupPersistenceFailed_ = false;
  CN0349Calibration calibrations_[4]{};
  Conductivity::AdCalibration adCalibrations_[2]{};
  uint8_t calibrationSlot_ = 0;
  bool calibrationSampleStarted_ = false;
  uint32_t calibrationSlotStartedMs_ = 0, nextCalibrationId_ = 1, calibrationRunId_ = 0;
  MeasurementSensors activeSensor_ = MeasurementSensors::Pt100;
  MeasurementCycle cycle_{}, result_{};
  bool hasResult_ = false, resultPending_ = false;
  uint32_t nextCycleId_ = 1, lastCycleStartedMs_ = 0, sensorStartedMs_ = 0;
  uint32_t intervalExceededCount_ = 0, publishedCycles_ = 0, overwrittenResults_ = 0;
};
