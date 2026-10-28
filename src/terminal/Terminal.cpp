#include "Terminal.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

namespace {
const char* sensorsName(MeasurementSensors s) {
  return s == MeasurementSensors::Both ? "PT100 EC" : s == MeasurementSensors::Pt100 ? "PT100" : "EC";
}
const char* stateName(InstrumentState s) {
  switch (s) {
    case InstrumentState::NotInitialized: return "NOT_INITIALIZED";
    case InstrumentState::Calibrating: return "CALIBRATING";
    case InstrumentState::Idle: return "IDLE";
    case InstrumentState::Measuring: return "MEASURING";
    case InstrumentState::Stopping: return "STOPPING";
    case InstrumentState::Error: return "ERROR";
  }
  return "ERROR";
}
bool positiveInteger(const char* text, uint32_t& result) {
  if (!*text) return false;
  uint32_t value = 0;
  for (; *text; ++text) {
    if (*text < '0' || *text > '9') return false;
    const uint8_t digit = *text - '0';
    if (value > (0x7fffffffUL - digit) / 10) return false;
    value = value * 10 + digit;
  }
  if (!value) return false;
  result = value; return true;
}
const char* modelName(Conductivity::Model model) {
  switch (model) {
    case Conductivity::Model::Raw: return "RAW";
    case Conductivity::Model::SeriesR2Experimental: return "SERIES";
    case Conductivity::Model::AnalogDevices: return "ANALOG_DEVICES";
  }
  return "UNKNOWN";
}
const char* modelShortName(Conductivity::Model model) {
  return model == Conductivity::Model::AnalogDevices ? "AD" : model == Conductivity::Model::SeriesR2Experimental ? "SERIES" : "RAW";
}
const char* conductivityErrorName(Conductivity::Error error) {
  if (error == Conductivity::Error::CalibrationInvalid || error == Conductivity::Error::CalibrationMismatch)
    return "ERR CAL_INVALID";
  if (error == Conductivity::Error::NonPositiveAdmittance || error == Conductivity::Error::InvalidDenominator ||
      error == Conductivity::Error::InvalidMagnitude) return "ERR OUT_OF_RANGE";
  return "ERR MEASUREMENT_FAILED";
}
}
Terminal::Terminal(Stream& serial, CN0349Manager& instrument) : serial_(serial), instrument_(instrument) {}
void Terminal::begin() {
  serial_.println("ATLANTES CN0349");
  if (instrument_.getStatus().state == InstrumentState::Error) serial_.println("ERR HARDWARE");
}
const char* Terminal::errorName(InstrumentError error) {
  switch (error) {
    case InstrumentError::None: return "OK";
    case InstrumentError::Busy: return "ERR BUSY";
    case InstrumentError::InvalidArgument: return "ERR INVALID_ARGUMENT";
    case InstrumentError::Hardware: return "ERR HARDWARE";
    case InstrumentError::NotCalibrated: return "ERR NOT_CALIBRATED";
    case InstrumentError::Timeout: return "ERR TIMEOUT";
    case InstrumentError::MeasurementFailed: return "ERR MEASUREMENT_FAILED";
    case InstrumentError::Cancelled: return "ERR CANCELLED";
  }
  return "ERR MEASUREMENT_FAILED";
}
void Terminal::reply(InstrumentError error) { serial_.println(errorName(error)); }
void Terminal::update() {
  for (size_t i = 0; i < Config::TERMINAL_BYTES_PER_UPDATE && serial_.available() > 0; ++i) {
    const int c = serial_.read(); if (c < 0) break;
    consume(static_cast<char>(c));
  }
  const auto status = instrument_.getStatus();
  if (status.calibrationRunId != calibrationRunId_) {
    calibrationRunId_ = status.calibrationRunId;
    serial_.print(status.validCalibrations == 4 ? "CAL OK " : "CAL PARTIAL ");
    serial_.print(status.validCalibrations); serial_.println("/4");
    if (!status.hardwareReady) serial_.println("ERR HARDWARE");
    else {
      serial_.print("AD LOW "); serial_.println(status.adLowValid ? "OK" : "INVALID");
      serial_.print("AD HIGH "); serial_.println(status.adHighValid ? "OK" : "INVALID");
      serial_.println(status.validCalibrations == 4 ? "READY" : "READY LIMITED");
    }
  }
  MeasurementCycle cycle;
  if (instrument_.takeMeasurementResult(cycle) && !cycle.cancelled) printCycle(cycle);
  if (stopPending_ && status.state != InstrumentState::Stopping) {
    stopPending_ = false;
    serial_.println(status.hardwareReady ? "OK STOPPED" : "ERR HARDWARE");
  }
}
void Terminal::consume(char character) {
  if (character == '\n' && previousWasCr_) {
    previousWasCr_ = false;
    return;
  }
  previousWasCr_ = (character == '\r');
  if (character == '\r' || character == '\n') {
    if (!discardLine_) {
      input_[inputLength_] = '\0';
      execute();
    }
    inputLength_ = 0;
    discardLine_ = false;
    return;
  }
  if (discardLine_) {
    return;
  }
  if (character == '\b' || character == 0x7f) {
    if (inputLength_ > 0) {
      --inputLength_;
    }
    return;
  }
  const unsigned char byte = static_cast<unsigned char>(character);
  if ((byte < 0x20 && character != '\t') || byte >= 0x7f) {
    discardLine_ = true;
    serial_.println("ERR INVALID_ARGUMENT");
    return;
  }
  if (inputLength_ >= sizeof(input_) - 1) {
    discardLine_ = true;
    serial_.println("ERR INVALID_ARGUMENT");
    return;
  }
  input_[inputLength_++] = character;
}

void Terminal::execute() {
  // Normaliza en el mismo buffer: minusculas, sin espacios extremos o repetidos.
  size_t output = 0;
  bool pendingSpace = false;
  for (size_t index = 0; index < inputLength_; ++index) {
    const unsigned char character = static_cast<unsigned char>(input_[index]);
    if (character == ' ' || character == '\t') {
      pendingSpace = output > 0;
      continue;
    }
    if (pendingSpace) {
      input_[output++] = ' ';
      pendingSpace = false;
    }
    input_[output++] = static_cast<char>(tolower(character));
  }
  input_[output] = '\0';
  if (output == 0) {
    return;
  }

  char* args[6]{}; size_t count = 0; char* context = nullptr;
  for (char* token = strtok_r(input_, " ", &context); token; token = strtok_r(nullptr, " ", &context)) {
    if (count == 6) { reply(InstrumentError::InvalidArgument); return; }
    args[count++] = token;
  }
  if (count == 1 && !strcmp(args[0], "help")) {
    serial_.println("help | status");
    serial_.println("setup show | setup interval <ms>");
    serial_.println("setup ec range high|low | setup ec model raw|series|ad | setup ec k <K>");
    serial_.println("cal status | cal show | cal run");
    serial_.println("measure show | measure set PT100|EC|PT100 EC | measure once|start|stop");
    return;
  }
  if (count == 1 && !strcmp(args[0], "status")) {
    const auto s = instrument_.getStatus();
    serial_.print(!s.hardwareReady ? "ERROR" : s.validCalibrations == 4 ? "READY" : "READY LIMITED");
    serial_.print(" | "); serial_.print(sensorsName(instrument_.getSetup().sensors));
    serial_.print(" | "); serial_.print(stateName(s.state));
    serial_.print(" | CAL "); serial_.print(s.validCalibrations); serial_.println("/4"); return;
  }
  if (count == 2 && !strcmp(args[0], "cal")) {
    if (!strcmp(args[1], "status")) {
      uint8_t legacyValid = 0;
      for (uint8_t i = 0; i < 4; ++i) {
        CN0349Calibration c; instrument_.getCalibrationStatus(static_cast<CN0349Preset>(i), c);
        if (c.compatibleWithCurrentConfig) ++legacyValid;
      }
      serial_.print("LEGACY "); serial_.print(legacyValid); serial_.print("/4 ");
      serial_.println(legacyValid == 4 ? "OK" : "PARTIAL");
      Conductivity::AdCalibration ad;
      instrument_.getAdCalibrationStatus(CN0349EcRange::Low, ad);
      serial_.print("AD LOW "); serial_.println(ad.valid ? "OK" : "ERR CAL_INVALID");
      instrument_.getAdCalibrationStatus(CN0349EcRange::High, ad);
      serial_.print("AD HIGH "); serial_.println(ad.valid ? "OK" : "ERR CAL_INVALID");
      return;
    }
    if (!strcmp(args[1], "show")) {
      static const char* names[] = {"HIGH_100R", "HIGH_1K", "LOW_1K", "LOW_10K"};
      for (uint8_t i = 0; i < 4; ++i) {
        CN0349Calibration c; instrument_.getCalibrationStatus(static_cast<CN0349Preset>(i), c);
        serial_.print("LEGACY "); serial_.print(names[i]); serial_.print(" | GF ");
        serial_.print(c.gainFactorPerOhm, 9); serial_.print(" 1/ohm/code | ID ");
        serial_.print(c.calibrationId); serial_.print(" | ");
        serial_.println(c.compatibleWithCurrentConfig ? "OK" : errorName(c.error));
      }
      const CN0349EcRange ranges[] = {CN0349EcRange::Low, CN0349EcRange::High};
      for (const auto range : ranges) {
        Conductivity::AdCalibration ad; instrument_.getAdCalibrationStatus(range, ad);
        serial_.print("AD "); serial_.print(range == CN0349EcRange::Low ? "LOW" : "HIGH");
        serial_.print(" | NL "); serial_.print(ad.nLow, 3);
        serial_.print(" | NH "); serial_.print(ad.nHigh, 3);
        serial_.print(" | GF "); serial_.print(ad.gainFactorSiemensPerCode, 12);
        serial_.print(" S/code | NOS "); serial_.print(ad.systemOffsetCode, 3);
        serial_.print(" | ID "); serial_.print(ad.calibrationId); serial_.print(" | ");
        serial_.println(ad.valid ? "OK" : "ERR CAL_INVALID");
      }
      return;
    }
    if (!strcmp(args[1], "run")) { reply(instrument_.runCalibration()); return; }
  }
  if (count >= 2 && !strcmp(args[0], "measure")) {
    if (count == 2) {
      if (!strcmp(args[1], "show")) {
        serial_.print("SENSORS "); serial_.println(sensorsName(instrument_.getSetup().sensors)); return;
      }
      if (!strcmp(args[1], "once")) {
        const auto e = instrument_.startSingleMeasurement(); if (e != InstrumentError::None) reply(e); return;
      }
      if (!strcmp(args[1], "start")) {
        const auto e = instrument_.startContinuousMeasurement();
        if (e != InstrumentError::None) reply(e);
        else { serial_.print("OK CONTINUOUS "); serial_.print(instrument_.getSetup().intervalMs); serial_.println(" ms"); }
        return;
      }
      if (!strcmp(args[1], "stop")) {
        const auto e = instrument_.stopMeasurement();
        if (e != InstrumentError::None) reply(e); else stopPending_ = true;
        return;
      }
    }
    if (!strcmp(args[1], "set") && (count == 3 || count == 4)) {
      uint8_t mask = 0;
      for (size_t i = 2; i < count; ++i) {
        const uint8_t bit = !strcmp(args[i], "pt100") ? 1 : !strcmp(args[i], "ec") ? 2 : 0;
        if (!bit || (mask & bit)) { reply(InstrumentError::InvalidArgument); return; }
        mask |= bit;
      }
      const auto e = instrument_.setMeasurementSensors(static_cast<MeasurementSensors>(mask));
      if (e != InstrumentError::None) reply(e);
      else { serial_.print("OK SENSORS "); serial_.println(sensorsName(instrument_.getSetup().sensors)); }
      return;
    }
  }
  if (count >= 2 && !strcmp(args[0], "setup")) {
    auto setup = instrument_.getSetup();
    if (count == 2 && !strcmp(args[1], "show")) { printSetup(); return; }
    if (count == 3 && !strcmp(args[1], "interval")) {
      if (!positiveInteger(args[2], setup.intervalMs)) { reply(InstrumentError::InvalidArgument); return; }
    } else if (count == 4 && !strcmp(args[1], "ec")) {
      if (!strcmp(args[2], "range") && (!strcmp(args[3], "high") || !strcmp(args[3], "low"))) {
        setup.ecRange = !strcmp(args[3], "high") ? CN0349EcRange::High : CN0349EcRange::Low;
        setup.ecCalibration = setup.ecRange == CN0349EcRange::High ? CN0349Preset::CalHigh100R : CN0349Preset::CalLow10K;
      } else if (!strcmp(args[2], "model") && (!strcmp(args[3], "raw") || !strcmp(args[3], "series") ||
                 !strcmp(args[3], "ad") || !strcmp(args[3], "analog_devices"))) {
        setup.ecModel = !strcmp(args[3], "raw") ? Conductivity::Model::Raw : !strcmp(args[3], "series") ?
            Conductivity::Model::SeriesR2Experimental : Conductivity::Model::AnalogDevices;
      } else if (!strcmp(args[2], "k")) {
        char* end = nullptr; errno = 0;
        setup.nominalKPerCm = strtof(args[3], &end);
        if (errno || end == args[3] || *end || !isfinite(setup.nominalKPerCm) || setup.nominalKPerCm <= 0) {
          reply(InstrumentError::InvalidArgument); return;
        }
      } else { reply(InstrumentError::InvalidArgument); return; }
    } else { reply(InstrumentError::InvalidArgument); return; }
    reply(instrument_.setSetup(setup)); return;
  }
  reply(InstrumentError::InvalidArgument);
}
void Terminal::printSetup() {
  const auto s = instrument_.getSetup();
  serial_.print("SENSORS "); serial_.println(sensorsName(s.sensors));
  serial_.print("INTERVAL "); serial_.print(s.intervalMs); serial_.println(" ms");
  serial_.print("EC RANGE "); serial_.println(s.ecRange == CN0349EcRange::High ? "HIGH" : "LOW");
  serial_.print("EC MODEL "); serial_.println(modelName(s.ecModel));
  serial_.print("EC K "); serial_.print(s.nominalKPerCm, 3); serial_.println(" cm-1");
}
void Terminal::printCycle(const MeasurementCycle& c) {
  const bool pt = c.setup.sensors != MeasurementSensors::Ec, ec = c.setup.sensors != MeasurementSensors::Pt100;
  if (pt) {
    serial_.print("PT100 | ");
    if (c.pt100.valid) {
      serial_.print(c.pt100.temperature.celsius, 2); serial_.print(" C");
      if (!ec) { serial_.print(" | "); serial_.print(c.pt100.acquisition.apparentImpedanceOhms, 2); serial_.print(" ohm"); }
    } else serial_.print(errorName(c.pt100.error));
  }
  if (ec) {
    if (pt) serial_.print(" | "); serial_.print("EC | ");
    if (c.ec.valid) {
      serial_.print(c.ec.conductivity.microSiemensPerCm, 1); serial_.print(" uS/cm");
      serial_.print(" | "); serial_.print(modelShortName(c.setup.ecModel));
    } else {
      serial_.print(modelShortName(c.setup.ecModel)); serial_.print(" | ");
      serial_.print(c.ec.conductivity.error != Conductivity::Error::NotRun ?
          conductivityErrorName(c.ec.conductivity.error) : errorName(c.ec.error));
    }
  }
  if (c.valid) serial_.print(" | OK");
  serial_.println();
}
