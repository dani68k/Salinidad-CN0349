#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

class Terminal {
 public:
  Terminal(Stream& serial, TwoWire& wire);
  void begin(bool i2cReady);
  void update();
  void printInfo();
  using ModuleHandler = bool (*)(void*, char*, bool, bool);
  using ModuleBusy = bool (*)(void*);
  bool attachModule(void* context, ModuleHandler handler, ModuleBusy busy, const char* help);

 private:
  struct ScanResult {
    uint16_t probed = 0;
    uint16_t found = 0;
    uint16_t nack = 0;
    uint16_t errors = 0;
    uint8_t lastAddress = 0;
    uint8_t lastCode = 0;
    uint8_t lastErrorAddress = 0;
    uint8_t lastErrorCode = 0;
  };
  using Handler = void (Terminal::*)();
  struct Command {
    const char* name;
    const char* description;
    Handler handler;
  };
  static const Command commands_[];
  void consume(char character);
  void execute();
  void printHelp();
  void printI2cConfig();
  void printStatus();
  void clear();
  void startScan();
  void advanceScan();
  void printScanResult(const ScanResult& result);
  void printHex(uint8_t value);
  static const char* resultDescription(uint8_t code);

  Stream& serial_;
  struct Module {
    void* context = nullptr;
    ModuleHandler handler = nullptr;
    ModuleBusy busy = nullptr;
    const char* help = nullptr;
  };
  static constexpr size_t MAX_MODULES = 3;
  Module modules_[MAX_MODULES] = {};
  size_t moduleCount_ = 0;
  bool moduleBusy() const;
  TwoWire& wire_;
  char input_[Config::TERMINAL_BUFFER_SIZE] = {};
  size_t inputLength_ = 0;
  bool discardLine_ = false;
  bool previousWasCr_ = false;
  bool i2cReady_ = false;
  bool scanActive_ = false;
  bool hasCompletedScan_ = false;
  uint8_t nextAddress_ = Config::I2C_FIRST_ADDRESS;
  ScanResult currentScan_;
  ScanResult lastScan_;
};
