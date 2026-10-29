#include "BleApp.h"

#include <stdio.h>
#include <string.h>

void BleApp::ServerCallbacks::onConnect(NimBLEServer*, NimBLEConnInfo&) {
  owner_.connected_ = true;
}

void BleApp::ServerCallbacks::onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) {
  owner_.connected_ = false;
}

void BleApp::CommandCallbacks::onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo&) {
  const NimBLEAttValue value = characteristic->getValue();
  owner_.enqueueSetupCommand(value.data(), value.size());
}

void BleApp::ControlCallbacks::onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo&) {
  const NimBLEAttValue value = characteristic->getValue();
  owner_.enqueueControlCommand(value.data(), value.size());
}

const char* BleApp::stateName(const InstrumentStatus& status) {
  switch (status.state) {
    case InstrumentState::NotInitialized: return "INIT";
    case InstrumentState::Calibrating: return "CALIBRATING";
    case InstrumentState::Idle: return status.hardwareReady ? "READY" : "ERROR";
    case InstrumentState::Measuring:
    case InstrumentState::Stopping: return "MEASURING";
    case InstrumentState::Error: return "ERROR";
  }
  return "ERROR";
}

void BleApp::begin() {
  if (initialized_) return;
  NimBLEDevice::init(AtlantesBle::DEVICE_NAME);
  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(&callbacks_);
  server->advertiseOnDisconnect(true);

  NimBLEService* service = server->createService(AtlantesBle::SERVICE_UUID);
  statusCharacteristic_ = service->createCharacteristic(
      AtlantesBle::STATUS_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  setupCharacteristic_ = service->createCharacteristic(
      AtlantesBle::SETUP_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY,
      AtlantesBle::SETUP_SIZE);
  NimBLECharacteristic* commandCharacteristic = service->createCharacteristic(
      AtlantesBle::SETUP_COMMAND_UUID, NIMBLE_PROPERTY::WRITE,
      AtlantesBle::SETUP_COMMAND_SIZE);
  setupResultCharacteristic_ = service->createCharacteristic(
      AtlantesBle::SETUP_RESULT_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY,
      AtlantesBle::SETUP_RESULT_SIZE);
  calibrationCharacteristic_ = service->createCharacteristic(
      AtlantesBle::CALIBRATION_UUID, NIMBLE_PROPERTY::READ,
      AtlantesBle::CALIBRATION_SIZE);
  measurementCharacteristic_ = service->createCharacteristic(
      AtlantesBle::MEASUREMENT_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY,
      AtlantesBle::MEASUREMENT_SIZE);
  NimBLECharacteristic* controlCommandCharacteristic = service->createCharacteristic(
      AtlantesBle::CONTROL_COMMAND_UUID, NIMBLE_PROPERTY::WRITE,
      AtlantesBle::CONTROL_COMMAND_SIZE);
  controlResultCharacteristic_ = service->createCharacteristic(
      AtlantesBle::CONTROL_RESULT_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY,
      AtlantesBle::CONTROL_RESULT_SIZE);
  advancedCharacteristic_ = service->createCharacteristic(
      AtlantesBle::ADVANCED_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY,
      AtlantesBle::ADVANCED_SIZE);
  initialized_ = advancedCharacteristic_ && statusCharacteristic_ && setupCharacteristic_ && commandCharacteristic &&
      setupResultCharacteristic_ && calibrationCharacteristic_ && measurementCharacteristic_ &&
      controlCommandCharacteristic && controlResultCharacteristic_;
  if (!initialized_) return;
  commandCharacteristic->setCallbacks(&commandCallbacks_);
  controlCommandCharacteristic->setCallbacks(&controlCallbacks_);

  publishAdvanced();
  publishStatus(false);
  publishSetup(false);
  publishSetupResult(0, AtlantesBle::SetupResult::Ok);
  publishCalibration();
  publishControlResult(0, 0, AtlantesBle::ControlResult::Ok);
  uint8_t emptyMeasurement[AtlantesBle::MEASUREMENT_SIZE]{};
  emptyMeasurement[0] = AtlantesBle::PROTOCOL_VERSION;
  measurementCharacteristic_->setValue(emptyMeasurement, sizeof(emptyMeasurement));
  service->start();
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->setName(AtlantesBle::DEVICE_NAME);
  advertising->addServiceUUID(AtlantesBle::SERVICE_UUID);
  advertising->enableScanResponse(true);
  advertising->start();
}

void BleApp::update() {
  if (!initialized_) return;
  processSetupCommand();
  processControlCommand();
  publishCalibration();
  publishAdvanced();
  publishStatus(false);
  publishSetup(false);
  publishMeasurement();
}

void BleApp::publishAdvanced() {
  const ImpedanceApp::Profile profile = instrument_.getImpedanceProfile();
  uint8_t payload[AtlantesBle::ADVANCED_SIZE]{};
  payload[0] = AtlantesBle::PROTOCOL_VERSION;
  const uint32_t values[] = {profile.mclkHz, profile.frequencyHz, profile.frequencyIncrementHz};
  for (size_t i = 0; i < 3; ++i)
    for (size_t b = 0; b < 4; ++b) payload[1 + i * 4 + b] = static_cast<uint8_t>(values[i] >> (8 * b));
  payload[13] = static_cast<uint8_t>(profile.numberOfIncrements);
  payload[14] = static_cast<uint8_t>(profile.numberOfIncrements >> 8);
  payload[15] = static_cast<uint8_t>(profile.settlingCycles);
  payload[16] = static_cast<uint8_t>(profile.settlingCycles >> 8);
  payload[17] = profile.excitationRange;
  payload[18] = profile.pgaGain;
  payload[19] = profile.excitationRange == 1 ? 20 : 0;  // Nominal Vpp in tenths; 0 means unavailable.
  if (hasLastAdvanced_ && memcmp(payload, lastAdvanced_, sizeof(payload)) == 0) return;
  memcpy(lastAdvanced_, payload, sizeof(payload));
  hasLastAdvanced_ = true;
  advancedCharacteristic_->setValue(payload, sizeof(payload));
  if (connected_) advancedCharacteristic_->notify();
}

void BleApp::publishStatus(bool forceNotify) {
  const InstrumentStatus status = instrument_.getStatus();
  char payload[AtlantesBle::STATUS_CAPACITY]{};
  snprintf(payload, sizeof(payload), "{\"state\":\"%s\",\"cal\":%u,\"busy\":%s}",
      stateName(status), status.validCalibrations, instrument_.isBusy() ? "true" : "false");
  if (!forceNotify && strcmp(payload, lastStatus_) == 0) return;
  strncpy(lastStatus_, payload, sizeof(lastStatus_) - 1);
  statusCharacteristic_->setValue(payload);
  if (connected_) statusCharacteristic_->notify();
}

void BleApp::encodeSetup(const InstrumentSetup& setup, uint8_t* output) {
  output[0] = AtlantesBle::PROTOCOL_VERSION;
  output[1] = static_cast<uint8_t>(setup.sensors);
  output[2] = static_cast<uint8_t>(setup.intervalMs);
  output[3] = static_cast<uint8_t>(setup.intervalMs >> 8);
  output[4] = static_cast<uint8_t>(setup.intervalMs >> 16);
  output[5] = static_cast<uint8_t>(setup.intervalMs >> 24);
  output[6] = setup.ecRange == CN0349EcRange::High ? 1 : 0;
  output[7] = setup.ecModel == Conductivity::Model::Raw ? 0 :
      setup.ecModel == Conductivity::Model::SeriesR2Experimental ? 1 : 2;
  static_assert(sizeof(float) == 4, "BLE setup protocol requires IEEE-754 float32");
  memcpy(output + 8, &setup.nominalKPerCm, sizeof(float));
}

bool BleApp::decodeSetup(const uint8_t* input, InstrumentSetup& setup) {
  if (input[0] != AtlantesBle::PROTOCOL_VERSION || input[1] < 1 || input[1] > 3 ||
      input[6] > 1 || input[7] > 2) return false;
  setup.sensors = static_cast<MeasurementSensors>(input[1]);
  setup.intervalMs = static_cast<uint32_t>(input[2]) |
      (static_cast<uint32_t>(input[3]) << 8) | (static_cast<uint32_t>(input[4]) << 16) |
      (static_cast<uint32_t>(input[5]) << 24);
  setup.ecRange = input[6] ? CN0349EcRange::High : CN0349EcRange::Low;
  setup.ecCalibration = setup.ecRange == CN0349EcRange::High ?
      CN0349Preset::CalHigh100R : CN0349Preset::CalLow10K;
  setup.ecModel = input[7] == 0 ? Conductivity::Model::Raw : input[7] == 1 ?
      Conductivity::Model::SeriesR2Experimental : Conductivity::Model::AnalogDevices;
  memcpy(&setup.nominalKPerCm, input + 8, sizeof(float));
  return true;
}

void BleApp::publishSetup(bool forceNotify) {
  uint8_t payload[AtlantesBle::SETUP_SIZE]{};
  encodeSetup(instrument_.getSetup(), payload);
  if (!forceNotify && hasLastSetup_ && memcmp(payload, lastSetup_, sizeof(payload)) == 0) return;
  memcpy(lastSetup_, payload, sizeof(payload));
  hasLastSetup_ = true;
  setupCharacteristic_->setValue(payload, sizeof(payload));
  if (connected_) setupCharacteristic_->notify();
}

void BleApp::publishSetupResult(uint8_t requestId, AtlantesBle::SetupResult result) {
  const uint8_t payload[AtlantesBle::SETUP_RESULT_SIZE] = {
      AtlantesBle::PROTOCOL_VERSION, requestId, static_cast<uint8_t>(result)};
  setupResultCharacteristic_->setValue(payload, sizeof(payload));
  if (connected_) setupResultCharacteristic_->notify();
}

void BleApp::publishCalibration() {
  uint8_t payload[AtlantesBle::CALIBRATION_SIZE]{};
  const InstrumentStatus status = instrument_.getStatus();
  payload[0] = AtlantesBle::PROTOCOL_VERSION;
  payload[2] = static_cast<uint8_t>(status.calibrationRunId);
  payload[3] = static_cast<uint8_t>(status.calibrationRunId >> 8);
  payload[4] = static_cast<uint8_t>(status.calibrationRunId >> 16);
  payload[5] = static_cast<uint8_t>(status.calibrationRunId >> 24);
  if (status.state == InstrumentState::Calibrating) {
    payload[1] = 1;  // CALIBRATING: records intentionally absent until the run completes.
  } else if (status.calibrationRunId == 0) {
    payload[1] = 0;  // ABSENT.
  } else {
    uint8_t validReferences = 0;
    bool referenceValid[4]{};
    size_t offset = 6;
    for (uint8_t i = 0; i < 4; ++i) {
      CN0349Calibration calibration;
      if (instrument_.getCalibrationStatus(static_cast<CN0349Preset>(i), calibration)) {
        payload[offset] = static_cast<uint8_t>(calibration.state);
        memcpy(payload + offset + 1, &calibration.meanMagnitude, sizeof(float));
        memcpy(payload + offset + 5, &calibration.gainFactorPerOhm, sizeof(float));
        referenceValid[i] = calibration.valid && calibration.state == CalibrationState::Valid;
        if (referenceValid[i]) ++validReferences;
      }
      offset += 9;
    }
    uint8_t validAd = 0;
    const CN0349EcRange ranges[] = {CN0349EcRange::High, CN0349EcRange::Low};
    for (const auto range : ranges) {
      Conductivity::AdCalibration calibration;
      const bool sourcesValid = range == CN0349EcRange::High ?
          referenceValid[0] && referenceValid[1] : referenceValid[2] && referenceValid[3];
      if (instrument_.getAdCalibrationStatus(range, calibration) && sourcesValid && calibration.valid) {
        payload[offset] = 1;
        memcpy(payload + offset + 1, &calibration.gainFactorSiemensPerCode, sizeof(double));
        memcpy(payload + offset + 9, &calibration.systemOffsetCode, sizeof(double));
        ++validAd;
      }
      offset += 17;
    }
    payload[1] = validReferences == 4 && validAd == 2 ? 2 :
        (validReferences || validAd) ? 3 : 4;  // VALID, PARTIAL or INVALID.
  }
  if (hasLastCalibration_ && memcmp(payload, lastCalibration_, sizeof(payload)) == 0) return;
  memcpy(lastCalibration_, payload, sizeof(payload));
  hasLastCalibration_ = true;
  calibrationCharacteristic_->setValue(payload, sizeof(payload));
}

void BleApp::publishMeasurement() {
  const InstrumentStatus status = instrument_.getStatus();
  if (status.publishedCycles == 0 || status.publishedCycles == lastPublishedCycle_) return;
  MeasurementCycle cycle;
  if (!instrument_.getMeasurementResult(cycle)) return;
  uint8_t payload[AtlantesBle::MEASUREMENT_SIZE]{};
  payload[0] = AtlantesBle::PROTOCOL_VERSION;
  payload[1] = static_cast<uint8_t>(cycle.setup.sensors);
  payload[2] = static_cast<uint8_t>(cycle.cycleId);
  payload[3] = static_cast<uint8_t>(cycle.cycleId >> 8);
  payload[4] = static_cast<uint8_t>(cycle.cycleId >> 16);
  payload[5] = static_cast<uint8_t>(cycle.cycleId >> 24);
  payload[6] = cycle.valid ? 1 : cycle.cancelled ? 2 : 3;
  payload[7] = static_cast<uint8_t>(cycle.error);
  const bool ptSelected = (static_cast<uint8_t>(cycle.setup.sensors) & 1) != 0;
  const bool ecSelected = (static_cast<uint8_t>(cycle.setup.sensors) & 2) != 0;
  payload[8] = !ptSelected ? 0 : cycle.pt100.valid ? 1 : 2;
  payload[9] = static_cast<uint8_t>(cycle.pt100.error);
  const float celsius = static_cast<float>(cycle.pt100.temperature.celsius);
  memcpy(payload + 10, &celsius, sizeof(celsius));
  payload[14] = !ecSelected ? 0 : cycle.ec.valid ? 1 : 2;
  payload[15] = static_cast<uint8_t>(cycle.ec.error);
  payload[16] = static_cast<uint8_t>(cycle.setup.ecModel);
  payload[17] = static_cast<uint8_t>(cycle.ec.conductivity.error);
  memcpy(payload + 18, &cycle.ec.conductivity.microSiemensPerCm, sizeof(float));
  memcpy(payload + 22, &cycle.ec.conductivity.milliSiemensPerCm, sizeof(float));
  measurementCharacteristic_->setValue(payload, sizeof(payload));
  if (connected_) measurementCharacteristic_->notify();
  lastPublishedCycle_ = status.publishedCycles;
}

void BleApp::publishControlResult(uint8_t requestId, uint8_t operation,
                                  AtlantesBle::ControlResult result) {
  const uint8_t payload[AtlantesBle::CONTROL_RESULT_SIZE] = {
      AtlantesBle::PROTOCOL_VERSION, requestId, operation, static_cast<uint8_t>(result)};
  controlResultCharacteristic_->setValue(payload, sizeof(payload));
  if (connected_) controlResultCharacteristic_->notify();
}

AtlantesBle::ControlResult BleApp::controlResult(InstrumentError error) {
  using R = AtlantesBle::ControlResult;
  switch (error) {
    case InstrumentError::None: return R::Ok;
    case InstrumentError::Busy: return R::Busy;
    case InstrumentError::InvalidArgument: return R::InvalidArgument;
    case InstrumentError::Hardware: return R::Hardware;
    case InstrumentError::NotCalibrated: return R::NotCalibrated;
    case InstrumentError::Timeout: return R::Timeout;
    case InstrumentError::MeasurementFailed: return R::MeasurementFailed;
    case InstrumentError::Cancelled: return R::Cancelled;
    case InstrumentError::Persistence: return R::InternalError;
  }
  return R::InternalError;
}

void BleApp::enqueueControlCommand(const uint8_t* data, size_t size) {
  portENTER_CRITICAL(&controlMux_);
  if (controlPending_) {
    controlRejectedRequestId_ = size > 1 ? data[1] : 0;
    controlRejectedOperation_ = size > 2 ? data[2] : 0;
    controlRejectedPending_ = true;
  } else {
    controlCommandSize_ = size;
    const size_t copySize = size > sizeof(controlCommand_) ? sizeof(controlCommand_) : size;
    if (copySize) memcpy(controlCommand_, data, copySize);
    controlPending_ = true;
  }
  portEXIT_CRITICAL(&controlMux_);
}

void BleApp::processControlCommand() {
  uint8_t command[AtlantesBle::CONTROL_COMMAND_SIZE]{};
  size_t size = 0;
  bool rejected = false;
  uint8_t rejectedRequestId = 0, rejectedOperation = 0;
  portENTER_CRITICAL(&controlMux_);
  if (controlRejectedPending_) {
    rejected = true; rejectedRequestId = controlRejectedRequestId_;
    rejectedOperation = controlRejectedOperation_; controlRejectedPending_ = false;
  }
  if (controlPending_) {
    size = controlCommandSize_;
    const size_t copySize = size > sizeof(command) ? sizeof(command) : size;
    if (copySize) memcpy(command, controlCommand_, copySize);
    controlPending_ = false;
  }
  portEXIT_CRITICAL(&controlMux_);
  if (rejected) publishControlResult(rejectedRequestId, rejectedOperation,
      AtlantesBle::ControlResult::Busy);
  if (!size) return;
  const uint8_t requestId = size > 1 ? command[1] : 0;
  const uint8_t operation = size > 2 ? command[2] : 0;
  if (size != AtlantesBle::CONTROL_COMMAND_SIZE) {
    publishControlResult(requestId, operation, AtlantesBle::ControlResult::InvalidLength); return;
  }
  if (command[0] != AtlantesBle::PROTOCOL_VERSION) {
    publishControlResult(requestId, operation, AtlantesBle::ControlResult::UnsupportedVersion); return;
  }
  const auto op = static_cast<AtlantesBle::ControlOperation>(operation);
  const InstrumentStatus status = instrument_.getStatus();
  InstrumentError error = InstrumentError::InvalidArgument;
  if (op == AtlantesBle::ControlOperation::MeasureStop) {
    if (status.state == InstrumentState::NotInitialized || status.state == InstrumentState::Calibrating ||
        status.state == InstrumentState::Error) {
      publishControlResult(requestId, operation, AtlantesBle::ControlResult::NotReady); return;
    }
    if (!status.continuous && status.state != InstrumentState::Measuring &&
        status.state != InstrumentState::Stopping) {
      publishControlResult(requestId, operation, AtlantesBle::ControlResult::InvalidArgument); return;
    }
    error = instrument_.stopMeasurement();
  } else {
    if (status.state != InstrumentState::Idle || !status.hardwareReady || instrument_.isBusy()) {
      publishControlResult(requestId, operation, status.state == InstrumentState::Idle ?
          AtlantesBle::ControlResult::Busy : AtlantesBle::ControlResult::NotReady); return;
    }
    switch (op) {
      case AtlantesBle::ControlOperation::CalRun: error = instrument_.runCalibration(); break;
      case AtlantesBle::ControlOperation::SetSensors:
        error = command[3] >= 1 && command[3] <= 3 ?
            instrument_.setMeasurementSensors(static_cast<MeasurementSensors>(command[3])) :
            InstrumentError::InvalidArgument;
        break;
      case AtlantesBle::ControlOperation::MeasureOnce: error = instrument_.startSingleMeasurement(); break;
      case AtlantesBle::ControlOperation::MeasureStart: error = instrument_.startContinuousMeasurement(); break;
      case AtlantesBle::ControlOperation::MeasureStop: break;
      default:
        publishControlResult(requestId, operation, AtlantesBle::ControlResult::InvalidOperation); return;
    }
  }
  publishControlResult(requestId, operation, controlResult(error));
}

void BleApp::enqueueSetupCommand(const uint8_t* data, size_t size) {
  const uint8_t requestId = size > 1 ? data[1] : 0;
  portENTER_CRITICAL(&commandMux_);
  if (commandPending_) {
    rejectedRequestId_ = requestId;
    rejectedPending_ = true;
    portEXIT_CRITICAL(&commandMux_);
    return;
  }
  commandSize_ = size;
  const size_t copySize = size > sizeof(command_) ? sizeof(command_) : size;
  if (copySize) memcpy(command_, data, copySize);
  commandPending_ = true;
  portEXIT_CRITICAL(&commandMux_);
}

void BleApp::processSetupCommand() {
  bool rejected = false, hasCommand = false;
  uint8_t rejectedRequestId = 0, requestId = 0, command[AtlantesBle::SETUP_COMMAND_SIZE]{};
  size_t size = 0;
  portENTER_CRITICAL(&commandMux_);
  if (rejectedPending_) {
    rejected = true; rejectedRequestId = rejectedRequestId_; rejectedPending_ = false;
  }
  if (commandPending_) {
    hasCommand = true; size = commandSize_;
    if (size > 1) requestId = command_[1];
    if (size == sizeof(command)) memcpy(command, command_, sizeof(command));
    commandPending_ = false;
  }
  portEXIT_CRITICAL(&commandMux_);
  if (rejected) publishSetupResult(rejectedRequestId, AtlantesBle::SetupResult::Busy);
  if (!hasCommand) return;
  if (size != AtlantesBle::SETUP_COMMAND_SIZE) {
    publishSetupResult(requestId, AtlantesBle::SetupResult::InvalidLength); return;
  }
  if (command[0] != AtlantesBle::PROTOCOL_VERSION) {
    publishSetupResult(requestId, AtlantesBle::SetupResult::UnsupportedVersion); return;
  }
  const InstrumentStatus status = instrument_.getStatus();
  if (status.state != InstrumentState::Idle || !status.hardwareReady) {
    publishSetupResult(requestId, AtlantesBle::SetupResult::NotReady); return;
  }
  if (instrument_.isBusy()) {
    publishSetupResult(requestId, AtlantesBle::SetupResult::Busy); return;
  }
  InstrumentSetup setup = instrument_.getSetup();
  uint8_t setupPayload[AtlantesBle::SETUP_SIZE]{};
  setupPayload[0] = command[0];
  memcpy(setupPayload + 1, command + 2, AtlantesBle::SETUP_SIZE - 1);
  if (!decodeSetup(setupPayload, setup)) {
    publishSetupResult(requestId, AtlantesBle::SetupResult::InvalidValue); return;
  }
  const InstrumentError error = instrument_.setSetup(setup);
  if (error == InstrumentError::None) {
    publishSetup(true);
    publishSetupResult(requestId, AtlantesBle::SetupResult::Ok);
  } else if (error == InstrumentError::Busy) {
    publishSetupResult(requestId, AtlantesBle::SetupResult::Busy);
  } else if (error == InstrumentError::InvalidArgument) {
    publishSetupResult(requestId, AtlantesBle::SetupResult::InvalidValue);
  } else {
    publishSetupResult(requestId, AtlantesBle::SetupResult::InternalError);
  }
}
