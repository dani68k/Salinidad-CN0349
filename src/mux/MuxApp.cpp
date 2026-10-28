#include "MuxApp.h"
#include <ctype.h>
#include <string.h>
const MuxApp::Preset MuxApp::presets_[] = {
    {MuxApp::CalibrationPreset::CalHigh100R, "CAL_HIGH_100R", 0x09, 100, 100, "RFB=100 ohm, RCAL=100 ohm"},
    {MuxApp::CalibrationPreset::CalHigh1K, "CAL_HIGH_1K", 0x11, 100, 1000, "RFB=100 ohm, RCAL=1 kohm"},
    {MuxApp::CalibrationPreset::CalLow1K, "CAL_LOW_1K", 0x12, 1000, 1000, "RFB=1 kohm, RCAL=1 kohm"},
    {MuxApp::CalibrationPreset::CalLow10K, "CAL_LOW_10K", 0x22, 1000, 10000, "RFB=1 kohm, RCAL=10 kohm"},
    {static_cast<MuxApp::CalibrationPreset>(0xFF), "RTD", 0x41, 100, 0, "RFB=100 ohm, DUT=RTD"},
    {static_cast<MuxApp::CalibrationPreset>(0xFF), "MEASURE_HIGH", 0x81, 100, 0, "RFB=100 ohm, DUT=conductivity cell"},
    {static_cast<MuxApp::CalibrationPreset>(0xFF), "MEASURE_LOW", 0x82, 1000, 0, "RFB=1 kohm, DUT=conductivity cell"},
    {static_cast<MuxApp::CalibrationPreset>(0xFF), "ALL_OFF", 0x00, 0, 0, "todos los interruptores abiertos"},
};

MuxApp::MuxApp(ADG715& mux, AD5934& impedance) : mux_(mux), impedance_(impedance) {}
bool MuxApp::selectPreset(const char* name) {
  const Preset* preset = presetByName(name);
  uint8_t actual = 0;
  if (!preset || !readMask(actual)) return false;
  return actual == preset->mask || setAuthorized(preset->mask);
}

bool MuxApp::readMask(uint8_t& mask) {
  if (!mux_.readState(mask)) return false;
  observedMask_ = mask;
  hasObservedMask_ = true;
  return true;
}

bool MuxApp::resolveCalibrationPreset(CalibrationPreset id, CalibrationInfo& info) const {
  for (const Preset& candidate : presets_) {
    if (candidate.id == id && id != static_cast<CalibrationPreset>(0xFF)) { info = candidate; return true; }
  }
  return false;
}

bool MuxApp::applyCalibrationPreset(CalibrationPreset id) {
  CalibrationInfo info{};
  if (!resolveCalibrationPreset(id, info)) return false;
  return selectPreset(info.name);
}

const MuxApp::Preset* MuxApp::presetByName(const char* name) const {
  for (const Preset& preset : presets_) {
    size_t index = 0;
    while (name[index] && preset.name[index] &&
           toupper(static_cast<unsigned char>(name[index])) == preset.name[index]) ++index;
    if (name[index] == '\0' && preset.name[index] == '\0') return &preset;
  }
  return nullptr;
}

const MuxApp::Preset* MuxApp::presetByMask(uint8_t mask) const {
  for (const Preset& preset : presets_) if (preset.mask == mask) return &preset;
  return nullptr;
}

bool MuxApp::impedanceIsSafe() {
  uint16_t control = 0;
  if (!impedance_.readRegister16(AD5934::CONTROL_HIGH, control)) return false;
  const uint8_t mode = static_cast<uint8_t>((control >> 12) & 0x0F);
  return (mode == 0x0A || mode == 0x0B) && (control & 0xFF) == 0x08;
}
bool MuxApp::setAuthorized(uint8_t mask) {
  if (!presetByMask(mask) || !impedanceIsSafe() || !mux_.writeState(mask)) return false;
  observedMask_ = mask; hasObservedMask_ = true; return true;
}

bool MuxApp::pt100Info(CalibrationInfo& info) const {
  const Preset* preset = presetByName("RTD");
  if (!preset) return false;
  info = *preset;
  return true;
}

bool MuxApp::applyPt100() { return selectPreset("RTD"); }

bool MuxApp::ecInfo(uint8_t mask, CalibrationInfo& info) const {
  if (mask != 0x81 && mask != 0x82) return false;
  const Preset* preset = presetByMask(mask);
  if (!preset) return false;
  info = *preset;
  return true;
}

bool MuxApp::applyEc(uint8_t mask) {
  CalibrationInfo info{};
  return ecInfo(mask, info) && selectPreset(info.name);
}
