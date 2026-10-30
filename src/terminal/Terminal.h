#pragma once
/** @file Terminal.h
 * @brief Interfaz de comandos y resultados del instrumento mediante Stream.
 */
#include <Arduino.h>
#include "config.h"
#include "../cn0349/CN0349Manager.h"
/** @brief Procesa órdenes serie y presenta el estado del CN0349Manager.
 * @details Comparte el controlador con BLE y no accede directamente al bus
 * I²C. Consume el slot de resultados mediante CN0349Manager::takeMeasurementResult().
 */
class Terminal {
 public:
  /** @brief Vincula el flujo serie y el controlador compartido.
   * @param serial Stream que proporciona entrada y salida; debe permanecer válido.
   * @param instrument Controlador compartido que debe permanecer válido.
   */
  Terminal(Stream& serial, CN0349Manager& instrument);
  /** @brief Imprime la cabecera inicial y, si corresponde, el error de hardware. */
  void begin();
  /** @brief Procesa entrada disponible y presenta cambios y resultados.
   * @details Limita los caracteres procesados por llamada según Config y
   * consume los resultados pendientes; los ciclos cancelados no se imprimen.
   */
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
