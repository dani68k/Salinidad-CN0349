#pragma once
#include <AD5934.h>
#include "impedance_diagnostics.h"
#include "../mux/MuxApp.h"
class ImpedanceApp {
 public:
  struct Profile {
    uint32_t mclkHz = 1000000;
    uint32_t frequencyHz = 2000;
    uint16_t settlingCycles = 10;
    uint16_t startSettlingCycles = 10;
    uint16_t numberOfIncrements = 0;
    uint32_t frequencyIncrementHz = 0;
    MuxApp::CalibrationPreset calibrationPreset = MuxApp::CalibrationPreset::CalLow1K;
    uint8_t excitationRange = 1;
    uint8_t pgaGain = 1;
    bool externalClock = true;
  };
  enum class MeasurementState : uint8_t {
    Idle, Configuring, PreparingStandby, Initializing, Settling, Acquiring,
    ReadingReal, ReadingImaginary, Stopping, Complete, Error, Cancelled
  };
  struct Result {
    int16_t real = 0;
    int16_t imaginary = 0;
    double magnitude = 0;
    uint32_t frequencyHz = 0;
    double effectiveFrequencyHz = 0;
    uint32_t elapsedMs = 0;
    MuxApp::CalibrationInfo calibration{};
    double phaseDegrees = 0;
    bool valid = false;
  };
  struct ImpedanceStatus {
    bool busy = false;
    bool ad5934Communication = false;
    bool adg715Communication = false;
    bool configurationVerified = false;
    bool operational = false;
    bool diagnosticComplete = false;
    bool hardwareReady = false;
    bool lastOperationSuccessful = false;
    bool adg715MaskKnown = false;
    uint8_t adg715Mask = 0;
    uint32_t elapsedMs = 0;
    ImpedanceDiagnostics::ErrorCode activeError = ImpedanceDiagnostics::ErrorCode::None;
    ImpedanceDiagnostics::ErrorCode lastError = ImpedanceDiagnostics::ErrorCode::None;
  };
  ImpedanceApp(AD5934& driver, MuxApp& mux);
  void update();
  bool isBusy() const;
  MeasurementState measurementState() const { return measurementState_; }
  const Profile& profile() const { return profile_; }
  ImpedanceStatus statusSnapshot() const;
  bool singleResult(Result& result) const {
    if (!hasResult_ || measurementState_ != MeasurementState::Complete) return false;
    result = result_; return true;
  }
  bool configureSingle(MuxApp::CalibrationPreset preset, uint32_t frequencyHz, uint16_t settlingCycles);
  bool requestSingle(bool busReady) { return startMeasurement(busReady, 0); }
  bool requestPt100(bool busReady) { return startMeasurement(busReady, 0x41); }
  bool requestEc(bool busReady, uint8_t mask) { return startMeasurement(busReady, mask); }
  bool presetInfo(MuxApp::CalibrationPreset p, MuxApp::CalibrationInfo& info) const { return mux_.resolveCalibrationPreset(p, info); }
  bool cancelOperation();
  bool requestRecovery(bool busReady);
 private:
  bool startMeasurement(bool busReady, uint8_t routeMask);
  void advanceMeasurement();
  void failMeasurement(const char* message, bool recover);
  void finishStopping();
  void setMeasurementState(MeasurementState state);
  void recover();
  void recordError(ImpedanceDiagnostics::ErrorCode code, ImpedanceDiagnostics::Subsystem subsystem,
                   const char* detail, bool recoverable);
  void refreshPublicStatus();
  AD5934& driver_;
  MuxApp& mux_;
  Profile profile_;
  MeasurementState measurementState_ = MeasurementState::Idle;
  Result result_;
  bool hasResult_ = false;
  uint32_t activeStartHz_ = 0;
  uint16_t activeIncrements_ = 0;
  MuxApp::CalibrationInfo measurementCalibration_{};
  bool cancelled_ = false;
  uint8_t configPhase_ = 0;
  uint8_t verifyPhase_ = 0;
  uint32_t startCode_ = 0;
  uint32_t incrementCode_ = 0;
  uint32_t measurementStartedMs_ = 0;
  uint32_t acquisitionStartedMs_ = 0;
  uint32_t stateStartedMs_ = 0;
  uint32_t lastPollMs_ = 0;
  uint32_t measurementElapsedMs_ = 0;
  int16_t pendingReal_ = 0;
  int16_t pendingImaginary_ = 0;
  const char* measurementError_ = nullptr;
  AD5934::Error driverError_;
  ImpedanceStatus publicStatus_{};
  ImpedanceDiagnostics::ErrorRecord lastErrorRecord_{};
  uint32_t errorCounters_[static_cast<uint8_t>(ImpedanceDiagnostics::ErrorCode::Count)]{};
};
