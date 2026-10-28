#pragma once
#include <Arduino.h>
#include "config.h"
#include "../cn0349/CN0349Manager.h"
class Terminal {
 public:
  Terminal(Stream& serial, CN0349Manager& instrument);
  void begin();
  void update();
 private:
  void consume(char character);
  void execute();
  void printSetup();
  void printCycle(const MeasurementCycle& cycle);
  void reply(InstrumentError error);
  static const char* errorName(InstrumentError error);
  Stream& serial_;
  CN0349Manager& instrument_;
  char input_[Config::TERMINAL_BUFFER_SIZE]{};
  size_t inputLength_ = 0;
  bool discardLine_ = false, previousWasCr_ = false, stopPending_ = false;
  uint32_t calibrationRunId_ = 0;
};
