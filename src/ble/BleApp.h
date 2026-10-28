#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "../cn0349/CN0349Manager.h"

namespace AtlantesBle {
constexpr char DEVICE_NAME[] = "CN0349";
constexpr char SERVICE_UUID[] = "8d7b0001-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr char STATUS_UUID[] = "8d7b0002-6f7a-4f31-9c4d-2b6e6a7f3a10";
constexpr size_t STATUS_CAPACITY = 80;
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

  static const char* stateName(const InstrumentStatus& status);
  void publishStatus(bool forceNotify);

  CN0349Manager& instrument_;
  ServerCallbacks callbacks_{*this};
  NimBLECharacteristic* statusCharacteristic_ = nullptr;
  bool initialized_ = false;
  volatile bool connected_ = false;
  char lastStatus_[AtlantesBle::STATUS_CAPACITY]{};
};
