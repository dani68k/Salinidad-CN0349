#pragma once

#include <AD5934.h>
#include <ADG715.h>
#include <Arduino.h>


class MuxApp {
 public:
  enum class CalibrationPreset : uint8_t { CalHigh100R, CalHigh1K, CalLow1K, CalLow10K };
  struct Preset {
    CalibrationPreset id;
    const char* name;
    uint8_t mask;
    uint16_t rfbOhms;
    uint16_t rcalOhms;
    const char* description;
  };
  using CalibrationInfo = Preset;
  MuxApp(ADG715& mux, AD5934& impedance);
  bool selectPreset(const char* name);
  bool resolveCalibrationPreset(CalibrationPreset id, CalibrationInfo& info) const;
  bool pt100Info(CalibrationInfo& info) const;
  bool applyPt100();
  bool ecInfo(uint8_t mask, CalibrationInfo& info) const;
  bool applyEc(uint8_t mask);
  bool applyCalibrationPreset(CalibrationPreset id);
  bool ping() { return mux_.begin(); }
  bool readMask(uint8_t& mask);
  bool hasObservedMask() const { return hasObservedMask_; }
  uint8_t observedMask() const { return observedMask_; }
  ADG715::Error lastDriverError() const { return mux_.lastError(); }
  const char* lastErrorText() const { return ADG715::errorText(mux_.lastError()); }
  uint8_t lastWireError() const { return mux_.getLastWireError(); }
  bool knownPresetMask(uint8_t mask) const { return presetByMask(mask) != nullptr; }
  uint8_t address() const { return mux_.address(); }

 private:
  static const Preset presets_[];
  bool setAuthorized(uint8_t mask);
  bool impedanceIsSafe();
  const Preset* presetByName(const char* name) const;
  const Preset* presetByMask(uint8_t mask) const;
  ADG715& mux_;
  AD5934& impedance_;
  bool hasObservedMask_ = false;
  uint8_t observedMask_ = 0;
};
