#pragma once
/** @file ADG715.h
 * @brief Driver I²C del registro de conmutación ADG715.
 */

#include <Wire.h>
#include <stdint.h>

/** @brief Controla los ocho interruptores del ADG715 por I²C.
 * @details No inicializa Wire ni aplica la política de rutas CN0349. Un
 * readback confirma el registro digital, no la continuidad analógica.
 */
class ADG715 {
 public:
  static constexpr uint8_t DEFAULT_ADDRESS = 0x48; ///< Dirección I²C predeterminada de 7 bits.
  /** @brief Códigos de error de validación, comunicación y verificación. */
  enum class Error : uint8_t {
    None, InvalidAddress, InvalidChannel, TxBuffer, AddressNack,
    DataNack, Timeout, Bus, ShortRead, VerifyMismatch
  };

  /** @brief Vincula un bus Wire ya inicializado y una dirección I²C.
   * @param wire Bus que debe permanecer válido durante la vida del driver.
   * @param address Dirección I²C de 7 bits del dispositivo.
   */
  explicit ADG715(TwoWire& wire, uint8_t address = DEFAULT_ADDRESS);
  /** @brief Sondea ACK sin escribir ni asumir un estado.
   * @return true si el dispositivo responde; false ante dirección o comunicación inválidas.
   */
  bool begin();
  /** @brief Lee el byte de control real del ADG715.
   * @param[out] state Máscara leída; no se modifica si la lectura falla.
   * @return true si se leyó; false en caso de error.
   */
  bool readState(uint8_t& state);
  /** @brief Escribe una máscara genérica y verifica el registro leído.
   * @param state Máscara de interruptores: bit 0 corresponde a S1 y bit 7 a S8; 1 cierra.
   * @return true si escritura y readback coinciden; false en caso de error.
   * @note Un fallo posterior a la escritura puede dejar el hardware modificado; no hay rollback.
   */
  bool writeState(uint8_t state);
  /** @brief Lee la máscara, cambia un interruptor y verifica la escritura.
   * @param channel Canal de 1 a 8.
   * @param enabled true para cerrar; false para abrir.
   * @return true si el nuevo estado se verificó; false si el canal o la operación fallan.
   */
  bool setSwitch(uint8_t channel, bool enabled);
  /** @brief Consulta un interruptor a partir de una lectura real.
   * @param channel Canal de 1 a 8.
   * @param[out] enabled Estado leído; no se modifica si falla la operación.
   * @return true si se leyó; false si el canal o la comunicación fallan.
   */
  bool getSwitch(uint8_t channel, bool& enabled);

  /** @brief Devuelve la dirección I²C configurada. */
  uint8_t address() const { return address_; }
  /** @brief Devuelve el último error como valor numérico de Error. */
  uint8_t getLastError() const { return static_cast<uint8_t>(lastError_); }
  /** @brief Devuelve el código Wire registrado con el último error. */
  uint8_t getLastWireError() const { return lastWireError_; }
  /** @brief Devuelve el último error registrado; una operación correcta no lo borra. */
  Error lastError() const { return lastError_; }
  /** @brief Indica si se ha solicitado una escritura de máscara. */
  bool hasRequestedState() const { return hasRequestedState_; }
  /** @brief Devuelve la última máscara solicitada; consultar hasRequestedState(). */
  uint8_t lastRequestedState() const { return lastRequestedState_; }
  /** @brief Indica si se ha confirmado una escritura mediante readback. */
  bool hasConfirmedState() const { return hasConfirmedState_; }
  /** @brief Devuelve la última máscara confirmada; consultar hasConfirmedState(). */
  uint8_t lastConfirmedState() const { return lastConfirmedState_; }
  /** @brief Obtiene una descripción textual de un error.
   * @param error Código a consultar.
   * @return Cadena estática que describe el error.
   */
  static const char* errorText(Error error);

 private:
  bool validAddress();
  bool setError(Error error, uint8_t wireCode = 0);
  TwoWire& wire_;
  uint8_t address_;
  Error lastError_ = Error::None;
  uint8_t lastWireError_ = 0;
  bool hasRequestedState_ = false;
  uint8_t lastRequestedState_ = 0;
  bool hasConfirmedState_ = false;
  uint8_t lastConfirmedState_ = 0;
};
