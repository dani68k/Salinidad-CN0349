#include "BleApp.h"

#include <stdio.h>
#include <string.h>

void BleApp::ServerCallbacks::onConnect(NimBLEServer*, NimBLEConnInfo&) {
  owner_.connected_ = true;
}

void BleApp::ServerCallbacks::onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) {
  owner_.connected_ = false;
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
  initialized_ = statusCharacteristic_ != nullptr;
  if (!initialized_) return;

  publishStatus(false);
  service->start();
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->setName(AtlantesBle::DEVICE_NAME);
  advertising->addServiceUUID(AtlantesBle::SERVICE_UUID);
  advertising->enableScanResponse(true);
  advertising->start();
}

void BleApp::update() {
  if (!initialized_) return;
  publishStatus(false);
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
