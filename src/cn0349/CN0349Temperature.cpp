#include "CN0349Manager.h"
#include <string.h>

CN0349ImpedanceError CN0349Manager::startTemperature(bool quiet) {
  if (isBusy()) return temperatureRequestError_ = CN0349ImpedanceError::Busy;
  temperatureResult_ = CN0349TemperatureResult{};
  const auto error = startImpedance(quiet);
  temperatureRequestError_ = error;
  if (error == CN0349ImpedanceError::None || error == CN0349ImpedanceError::AcquisitionFailed) {
    temperatureRunning_ = true;
    temperatureResult_.acquisition = getImpedanceResult();
    advanceTemperature();
  } else {
    temperatureResult_.acquisition.state = CN0349ImpedanceState::Failed;
    temperatureResult_.acquisition.error = error;
    // ID=0 explicitly means no calibration was applied to an acquisition.
    temperatureResult_.acquisition.calibrationPreset = impedanceCalibration();
  }
  return error;
}

void CN0349Manager::advanceTemperature() {
  if (!temperatureRunning_ || isImpedanceRunning()) return;
  temperatureResult_ = convertPt100Acquisition(getImpedanceResult());
  temperatureRunning_ = false;
  if (temperatureContinuous_) {
    printTemperatureSample();
    if (!temperatureResult_.valid) {
      temperatureContinuous_ = false;
      output_.println("Temperatura continua detenida: resultado invalido. Consulte cn0349 temperature result.");
    }
  }
}

CN0349ImpedanceError CN0349Manager::startContinuousTemperature() {
  const auto error = startTemperature(true);
  if (error == CN0349ImpedanceError::None) {
    temperatureContinuous_ = true;
    temperatureLastStartMs_ = millis();
    output_.println("Temperatura continua: intervalo 1000 ms. EXPERIMENTAL / NOT ABSOLUTELY CALIBRATED");
  }
  return error;
}

void CN0349Manager::scheduleTemperature() {
  if (!temperatureContinuous_ || temperatureRunning_ ||
      static_cast<uint32_t>(millis() - temperatureLastStartMs_) < 1000) return;
  // Temporarily release our reservation for the existing acquisition preflight.
  // Cooperative execution: no other request can run inside this call.
  temperatureContinuous_ = false;
  temperatureLastStartMs_ = millis();
  const auto error = startTemperature(true);
  if (error == CN0349ImpedanceError::None) temperatureContinuous_ = true;
  else {
    output_.print("Temperatura continua detenida; error="); output_.println(impedanceErrorName(error));
  }
}

void CN0349Manager::printTemperatureSample() {
  const auto& r = temperatureResult_;
  if (!r.valid) return;  // The caller reports the failure and stops the mode.
  output_.print("temperature: ");
  output_.print(r.conversion.celsius, 2);
  output_.println("\xC2\xBA" "C");
}

bool CN0349Manager::cancelTemperature() {
  const bool continuous = temperatureContinuous_;
  temperatureContinuous_ = false;
  const bool cancelled = temperatureRunning_ && cancelImpedance();
  return continuous || cancelled;
}

void CN0349Manager::printTemperatureStatus() {
  const auto& r = temperatureResult_;
  output_.println("EXPERIMENTAL / NOT ABSOLUTELY CALIBRATED");
  output_.print("Acquisition state/error: "); output_.print(impedanceStateName(r.acquisition.state));
  output_.print(" / "); output_.println(impedanceErrorName(r.acquisition.error));
  output_.print("Pt100 valid/conversion error: "); output_.print(r.valid ? "VALID" : "INVALID");
  output_.print(" / "); output_.println(Pt100::errorName(r.conversion.error));
  output_.print("Last temperature request error: "); output_.println(impedanceErrorName(temperatureRequestError_));
  output_.print("Temperature acquisition active: "); output_.println(temperatureRunning_ ? "yes" : "no");
  output_.print("Continuous temperature (1000 ms): "); output_.println(temperatureContinuous_ ? "yes" : "no");
}

bool CN0349Manager::processTemperature(char** args, size_t count, bool busReady, bool busBusy) {
  if (count == 3 && (strcmp(args[2], "start") == 0 || strcmp(args[2], "continuous") == 0)) {
    if (isBusy() || busBusy) temperatureRequestError_ = CN0349ImpedanceError::Busy;
    else if (!busReady) temperatureRequestError_ = CN0349ImpedanceError::NotReady;
    else if (strcmp(args[2], "continuous") == 0) startContinuousTemperature();
    else startTemperature();
    output_.print("Temperature request: "); output_.println(impedanceErrorName(temperatureRequestError_));
  } else if (count == 3 && strcmp(args[2], "stop") == 0) {
    output_.println(cancelTemperature() ? "Temperatura detenida; si hay adquisicion activa, esperando standby." : "No hay temperatura cancelable.");
  } else if (count == 3 && strcmp(args[2], "status") == 0) {
    printTemperatureStatus();
  } else if (count == 3 && strcmp(args[2], "result") == 0) {
    printTemperatureStatus();
    printImpedanceResult(temperatureResult_.acquisition);
    output_.print("Pt100 calculated temperature Celsius: ");
    if (temperatureResult_.valid) output_.println(temperatureResult_.conversion.celsius, 6);
    else output_.println("unavailable");
    output_.println("3 hilos como 2 terminales; sin compensacion de cables ni offset.");
  } else {
    output_.println("cn0349 temperature help|start|continuous|status|result|stop");
    output_.println("continuous: primera medida inmediata; siguientes cada 1000 ms, sin solapar; stop detiene.");
    output_.println("EXPERIMENTAL / NOT ABSOLUTELY CALIBRATED");
    output_.println("IEC 60751 Pt100: -200 a +850 Celsius; Z aparente sin compensacion de cables.");
    output_.println("Usa la seleccion cn0349 impedance cal set <CAL_HIGH_100R|CAL_HIGH_1K>.");
    output_.println("No recalibra; exige calibracion RAM compatible a 2000 Hz. ID=0: ninguna aplicada.");
  }
  return true;
}
