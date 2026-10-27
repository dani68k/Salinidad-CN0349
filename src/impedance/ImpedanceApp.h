#pragma once

#include <AD5934.h>
#include <Arduino.h>
#include <stdint.h>
#include "../../include/impedance_diagnostics.h"
#include "../mux/MuxApp.h"

class Terminal;

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
  struct SweepConfig {
    uint32_t startHz = 1000;
    uint32_t incrementHz = 250;
    uint16_t increments = 8;
  };
  enum class MeasurementState : uint8_t {
    Idle, Configuring, PreparingStandby, Initializing, Settling, Acquiring, ReadingReal,
    ReadingImaginary, Incrementing, Stopping, Complete, Error, Cancelled
  };
  struct Result {
    int16_t real = 0;
    int16_t imaginary = 0;
    double magnitude = 0;
    uint32_t frequencyHz = 0;
    double effectiveFrequencyHz = 0;
    uint32_t elapsedMs = 0;
    MuxApp::CalibrationInfo calibration{};
    uint16_t index = 0;
    double phaseDegrees = 0;
    bool valid = false;
  };
  struct SweepPoint {
    uint16_t index = 0;
    double frequencyHz = 0;
    int16_t real = 0;
    int16_t imaginary = 0;
    double magnitude = 0;
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
    uint16_t pointsCompleted = 0;
    uint16_t pointsExpected = 0;
    uint32_t elapsedMs = 0;
    ImpedanceDiagnostics::ErrorCode activeError = ImpedanceDiagnostics::ErrorCode::None;
    ImpedanceDiagnostics::ErrorCode lastError = ImpedanceDiagnostics::ErrorCode::None;
  };
  static constexpr uint16_t MAX_SWEEP_POINTS = 64;

  ImpedanceApp(AD5934& driver, MuxApp& mux, Stream& output);
  void begin(Terminal& terminal);
  void reportConnection(bool ready);
  void update();
  const ImpedanceStatus& publicStatus() const { return publicStatus_; }
  bool isBusy() const { return busy(const_cast<ImpedanceApp*>(this)); }
  MeasurementState measurementState() const { return measurementState_; }
  const Profile& profile() const { return profile_; }
  const SweepConfig& sweepConfiguration() const { return sweepConfig_; }
  const SweepConfig& activeSweepConfiguration() const { return activeSweepConfig_; }
  MuxApp::CalibrationPreset activeSweepPreset() const { return activeSweepCalibration_.id; }
  uint16_t activeSweepSettling() const { return activeSweepSettling_; }
  bool currentIsSweep() const { return sweepMode_; }
  bool hasSingleResult() const { return hasResult_ && result_.valid; }
  bool singleResult(Result& result) const { if (!hasSingleResult()) return false; result = result_; return true; }
  bool hasSweepResult() const { return hasSweep_; }
  bool sweepComplete() const { return sweepComplete_; }
  uint16_t sweepPointCount() const { return sweepPointCount_; }
  uint16_t sweepPointTarget() const { return sweepTargetPoints_; }
  bool sweepPoint(uint16_t index, SweepPoint& point) const { if (index >= sweepPointCount_) return false; point = sweepPoints_[index]; return true; }
  ImpedanceStatus statusSnapshot() const;
  bool configureSingle(MuxApp::CalibrationPreset preset, uint32_t frequencyHz, uint16_t settlingCycles);
  bool configureSweep(MuxApp::CalibrationPreset preset, uint32_t startHz, uint32_t stepHz,
                      uint16_t increments, uint16_t settlingCycles);
  bool setRequestedPreset(MuxApp::CalibrationPreset preset);
  bool requestSingle(bool busReady) { return startMeasurement(busReady, false); }
  bool requestPt100(bool busReady, bool quiet = false) { return startMeasurement(busReady, false, 0x41, quiet); }
  bool pt100Info(MuxApp::CalibrationInfo& info) const { return mux_.pt100Info(info); }
  bool ecInfo(uint8_t mask, MuxApp::CalibrationInfo& info) const { return mux_.ecInfo(mask, info); }
  bool requestEc(bool busReady, uint8_t mask) { return startMeasurement(busReady, false, mask); }
  bool requestSweep(bool busReady) { return startMeasurement(busReady, true); }
  bool cancelOperation();
  bool runDiagnostics(bool busReady);
  bool requestRecovery(bool busReady);
  bool presetInfo(MuxApp::CalibrationPreset preset, MuxApp::CalibrationInfo& info) const { return mux_.resolveCalibrationPreset(preset, info); }
  bool presetInfoForMask(uint8_t mask, MuxApp::CalibrationInfo& info) const;
  bool hasObservedMuxMask() const { return mux_.hasObservedMask(); }
  uint8_t observedMuxMask() const { return mux_.observedMask(); }

 private:
  static bool dispatch(void* context, char* line, bool busReady, bool busBusy);
  static bool busy(void* context);
  bool process(char* line, bool busReady, bool busBusy);
  static bool parseHex(const char* token, uint32_t maximum, uint32_t& result);
  static bool parseDecimal(const char* token, uint32_t maximum, uint32_t& result);
  bool read(uint8_t reg, uint8_t width, uint32_t& value);
  void write(uint8_t reg, uint8_t width, uint32_t value);
  void help();
  void measureHelp();
  void measureConfig();
  void measureStatus();
  void measureResult();
  void sweepHelp();
  void sweepConfig();
  void sweepStatus();
  void sweepResult();
  void printSweepPoint(const SweepPoint& point);
  void printCalibration(const MuxApp::CalibrationInfo& calibration);
  bool startMeasurement(bool busReady, bool sweep = false, uint8_t routeMask = 0, bool quiet = false);
  bool validateSweep(uint32_t& finalHz, uint32_t& startCode, uint32_t& incrementCode) const;
  void advanceMeasurement();
  void failMeasurement(const char* message, bool recover);
  void finishStopping();
  void printMeasurementError();
  void printOutput();
  void setMeasurementState(MeasurementState state);
  const char* measurementStateName() const;
  void status();
  void integratedStatus();
  void diagnostics();
  void errors(bool clear);
  void recover();
  void recordError(ImpedanceDiagnostics::ErrorCode code, ImpedanceDiagnostics::Subsystem subsystem,
                   const char* detail, bool recoverable);
  void refreshPublicStatus();
  void error();
  void printHex(uint32_t value, uint8_t digits);
  void printValue(uint8_t reg, uint8_t width, uint32_t value);
  AD5934& driver_;
  MuxApp& mux_;
  Stream& output_;
  bool dumpActive_ = false;
  size_t dumpGroup_ = 0;
  uint8_t dumpOffset_ = 0;
  Profile profile_;
  MeasurementState measurementState_ = MeasurementState::Idle;
  Result result_;
  bool hasResult_ = false;
  bool sweepMode_ = false;
  bool hasSweep_ = false;
  bool sweepComplete_ = false;
  SweepConfig sweepConfig_;
  SweepConfig activeSweepConfig_;
  SweepPoint sweepPoints_[MAX_SWEEP_POINTS]{};
  uint16_t sweepPointCount_ = 0;
  uint16_t sweepTargetPoints_ = 0;
  uint16_t sweepCurrentIndex_ = 0;
  uint32_t sweepEndHz_ = 0;
  uint32_t sweepElapsedMs_ = 0;
  const char* sweepError_ = nullptr;
  uint32_t activeStartHz_ = 0;
  uint32_t activeIncrementHz_ = 0;
  uint16_t activeIncrements_ = 0;
  uint32_t activeSweepMclkHz_ = 0;
  uint16_t activeSweepSettling_ = 0;
  MuxApp::CalibrationInfo measurementCalibration_{};
  MuxApp::CalibrationInfo activeSweepCalibration_{};
  bool cancelled_ = false;
  bool measurementQuiet_ = false;
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
  bool communicationAvailable_ = false;
  bool adgCommunicationAvailable_ = false;
  bool recoveryAttempted_ = false;
};
