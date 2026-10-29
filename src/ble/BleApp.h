#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "../cn0349/CN0349Manager.h"

namespace AtlantesBle {
constexpr char DEVICE_NAME[] = "CN0349";
constexpr char SERVICE_UUID[] = "8d7b0001-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr char STATUS_UUID[] = "8d7b0002-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr char SETUP_UUID[] = "8d7b0003-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr char SETUP_COMMAND_UUID[] = "8d7b0004-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr char SETUP_RESULT_UUID[] = "8d7b0005-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr char CALIBRATION_UUID[] = "8d7b0006-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr char MEASUREMENT_UUID[] = "8d7b0007-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr char CONTROL_COMMAND_UUID[] = "8d7b0008-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr char CONTROL_RESULT_UUID[] = "8d7b0009-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr char ADVANCED_UUID[] = "8d7b000a-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr char EC_DIAGNOSTIC_UUID[] = "8d7b000b-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr size_t EC_DIAGNOSTIC_SIZE = 7;
constexpr size_t ADVANCED_SIZE = 20;
constexpr size_t STATUS_CAPACITY = 80;
constexpr size_t SETUP_SIZE = 12;
constexpr size_t SETUP_COMMAND_SIZE = 13;
constexpr size_t SETUP_RESULT_SIZE = 3;
constexpr size_t CALIBRATION_SIZE = 76;
constexpr size_t MEASUREMENT_SIZE = 26;
constexpr size_t CONTROL_COMMAND_SIZE = 4;
constexpr size_t CONTROL_RESULT_SIZE = 4;
constexpr uint8_t PROTOCOL_VERSION = 1;

enum class SetupResult : uint8_t {
  Ok = 0, InvalidLength = 1, UnsupportedVersion = 2, InvalidValue = 3,
  NotReady = 4, Busy = 5, InternalError = 6
};
enum class ControlOperation : uint8_t {
  CalRun = 1, SetSensors = 2, MeasureOnce = 3, MeasureStart = 4, MeasureStop = 5
};
enum class ControlResult : uint8_t {
  Ok = 0, InvalidLength = 1, UnsupportedVersion = 2, InvalidOperation = 3,
  NotReady = 4, Busy = 5, InvalidArgument = 6, Hardware = 7,
  NotCalibrated = 8, Timeout = 9, MeasurementFailed = 10, Cancelled = 11,
  InternalError = 12
};
}

class BleApp {
 public:
  explicit BleApp(CN0349Manager& instrument) : instrument_(instrument) {}
  void begin();
  void update();

 private:
  class ServerCallbacks : public NimBLEServerCallbacks {
   public:
    explicit ServerCallbacks(BleApp& owner) : owner_(owner) {}
    void onConnect(NimBLEServer*, NimBLEConnInfo&) override;
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override;
   private:
    BleApp& owner_;
  };

  class CommandCallbacks : public NimBLECharacteristicCallbacks {
   public:
    explicit CommandCallbacks(BleApp& owner) : owner_(owner) {}
    void onWrite(NimBLECharacteristic*, NimBLEConnInfo&) override;
   private:
    BleApp& owner_;
  };
  class ControlCallbacks : public NimBLECharacteristicCallbacks {
   public:
    explicit ControlCallbacks(BleApp& owner) : owner_(owner) {}
    void onWrite(NimBLECharacteristic*, NimBLEConnInfo&) override;
   private:
    BleApp& owner_;
  };

  static const char* stateName(const InstrumentStatus& status);
  void publishAdvanced();
  void publishStatus(bool forceNotify);
  void publishSetup(bool forceNotify);
  void publishSetupResult(uint8_t requestId, AtlantesBle::SetupResult result);
  void publishCalibration();
  void publishMeasurement();
  void publishControlResult(uint8_t requestId, uint8_t operation, AtlantesBle::ControlResult result);
  void enqueueSetupCommand(const uint8_t* data, size_t size);
  void processSetupCommand();
  void enqueueControlCommand(const uint8_t* data, size_t size);
  void processControlCommand();
  static AtlantesBle::ControlResult controlResult(InstrumentError error);
  static void encodeSetup(const InstrumentSetup& setup, uint8_t* output);
  static bool decodeSetup(const uint8_t* input, InstrumentSetup& setup);

  CN0349Manager& instrument_;
  ServerCallbacks callbacks_{*this};
  CommandCallbacks commandCallbacks_{*this};
  ControlCallbacks controlCallbacks_{*this};
  NimBLECharacteristic* advancedCharacteristic_ = nullptr;
  uint8_t lastAdvanced_[AtlantesBle::ADVANCED_SIZE]{};
  bool hasLastAdvanced_ = false;
  NimBLECharacteristic* statusCharacteristic_ = nullptr;
  NimBLECharacteristic* setupCharacteristic_ = nullptr;
  NimBLECharacteristic* setupResultCharacteristic_ = nullptr;
  NimBLECharacteristic* calibrationCharacteristic_ = nullptr;
  NimBLECharacteristic* measurementCharacteristic_ = nullptr;
  NimBLECharacteristic* ecDiagnosticCharacteristic_ = nullptr;
  NimBLECharacteristic* controlResultCharacteristic_ = nullptr;
  bool initialized_ = false;
  volatile bool connected_ = false;
  volatile bool commandPending_ = false;
  volatile bool rejectedPending_ = false;
  volatile uint8_t rejectedRequestId_ = 0;
  portMUX_TYPE commandMux_ = portMUX_INITIALIZER_UNLOCKED;
  uint8_t command_[AtlantesBle::SETUP_COMMAND_SIZE]{};
  size_t commandSize_ = 0;
  char lastStatus_[AtlantesBle::STATUS_CAPACITY]{};
  uint8_t lastSetup_[AtlantesBle::SETUP_SIZE]{};
  bool hasLastSetup_ = false;
  uint8_t lastCalibration_[AtlantesBle::CALIBRATION_SIZE]{};
  bool hasLastCalibration_ = false;
  portMUX_TYPE controlMux_ = portMUX_INITIALIZER_UNLOCKED;
  bool disconnectStopPending_ = false;
  volatile bool controlPending_ = false;
  volatile bool controlRejectedPending_ = false;
  volatile uint8_t controlRejectedRequestId_ = 0;
  volatile uint8_t controlRejectedOperation_ = 0;
  uint8_t controlCommand_[AtlantesBle::CONTROL_COMMAND_SIZE]{};
  size_t controlCommandSize_ = 0;
  uint32_t lastPublishedCycle_ = 0;
};
