#pragma once
/** @file MuxApp.h
 * @brief Selección de rutas CN0349 mediante ADG715 e interlock AD5934.
 */

#include <AD5934.h>
#include <ADG715.h>
#include <Arduino.h>


/** @brief Aplica rutas autorizadas y verifica las condiciones de conmutación.
 * @details Una escritura de máscara exige AD5934 en estado inactivo seguro.
 * El readback del ADG715 confirma el registro digital, no la ruta analógica.
 */
class MuxApp {
 public:
  /** @brief Identifica las cuatro referencias de calibración electrónica. */
  enum class CalibrationPreset : uint8_t { CalHigh100R, CalHigh1K, CalLow1K, CalLow10K };
  /** @brief Información nominal de una ruta o referencia. */
  struct Preset {
    CalibrationPreset id; ///< Identificador de referencia cuando corresponde.
    const char* name; ///< Nombre interno de la ruta.
    uint8_t mask; ///< Máscara del registro ADG715.
    uint16_t rfbOhms; ///< Resistencia nominal de realimentación, en Ω.
    uint16_t rcalOhms; ///< Resistencia nominal de calibración, en Ω; cero para rutas de sensor.
    const char* description; ///< Descripción estática de la ruta.
  };
  /** @brief Tipo de información devuelta para referencias y rutas. */
  using CalibrationInfo = Preset;
  /** @brief Vincula los drivers de conmutación y de impedancia.
   * @param mux Driver ADG715 que debe permanecer válido.
   * @param impedance Driver AD5934 que debe permanecer válido.
   */
  MuxApp(ADG715& mux, AD5934& impedance);
  /** @brief Selecciona una ruta conocida por nombre, sin distinguir mayúsculas.
   * @param name Cadena terminada en cero con el nombre de la ruta.
   * @return true si la ruta ya estaba aplicada o se aplicó; false si no existe o falla la operación.
   * @note Si debe escribir una máscara, comprueba el interlock y verifica el readback.
   */
  bool selectPreset(const char* name);
  /** @brief Obtiene los datos de una referencia electrónica.
   * @param id Identificador de la referencia.
   * @param[out] info Copia de la información cuando existe.
   * @return true si se encontró; false si el identificador es inválido.
   */
  bool resolveCalibrationPreset(CalibrationPreset id, CalibrationInfo& info) const;
  /** @brief Obtiene los datos de la ruta Pt100.
   * @param[out] info Copia de la información de la ruta.
   * @return true si se encontró la ruta; false en caso contrario.
   */
  bool pt100Info(CalibrationInfo& info) const;
  /** @brief Selecciona la ruta Pt100; devuelve true si está aplicada. */
  bool applyPt100();
  /** @brief Obtiene los datos de una ruta EC.
   * @param mask Máscara 0x81 para HIGH o 0x82 para LOW.
   * @param[out] info Copia de la información si la máscara es válida.
   * @return true si se encontró la ruta; false si la máscara no es válida.
   */
  bool ecInfo(uint8_t mask, CalibrationInfo& info) const;
  /** @brief Selecciona una ruta EC.
   * @param mask Máscara 0x81 para HIGH o 0x82 para LOW.
   * @return true si está aplicada; false si falla la selección o la máscara es inválida.
   */
  bool applyEc(uint8_t mask);
  /** @brief Selecciona una referencia de calibración.
   * @param id Identificador de la referencia.
   * @return true si está aplicada; false si la referencia o la operación fallan.
   */
  bool applyCalibrationPreset(CalibrationPreset id);
  /** @brief Sondea el ADG715 sin cambiar su estado; devuelve true si responde. */
  bool ping() { return mux_.begin(); }
  /** @brief Lee el registro de control real del ADG715.
   * @param[out] mask Máscara leída cuando la operación tiene éxito.
   * @return true si se leyó; false si falló la comunicación.
   */
  bool readMask(uint8_t& mask);
  /** @brief Indica si se ha observado una máscara del ADG715. */
  bool hasObservedMask() const { return hasObservedMask_; }
  /** @brief Devuelve la última máscara observada; consultar hasObservedMask(). */
  uint8_t observedMask() const { return observedMask_; }
  /** @brief Devuelve el último error del driver ADG715. */
  ADG715::Error lastDriverError() const { return mux_.lastError(); }
  /** @brief Devuelve el texto del último error del driver ADG715. */
  const char* lastErrorText() const { return ADG715::errorText(mux_.lastError()); }
  /** @brief Devuelve el último código Wire registrado por el driver. */
  uint8_t lastWireError() const { return mux_.getLastWireError(); }
  /** @brief Comprueba si una máscara figura en el catálogo de rutas.
   * @param mask Máscara ADG715 a consultar.
   * @return true si es conocida; false en caso contrario.
   */
  bool knownPresetMask(uint8_t mask) const { return presetByMask(mask) != nullptr; }
  /** @brief Devuelve la dirección I²C del ADG715. */
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
