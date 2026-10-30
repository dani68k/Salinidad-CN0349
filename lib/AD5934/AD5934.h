#pragma once
/** @file AD5934.h
 * @brief Driver I²C de registros y control del convertidor de impedancia AD5934.
 */

#include <Wire.h>
#include <stdint.h>
#include "../../include/ad5934_math.h"

/** @brief Acceso protegido a registros y operaciones de control del AD5934.
 * @details No inicializa Wire ni convierte los datos DFT a impedancia o EC.
 * Las escrituras multibyte se realizan byte a byte y pueden quedar parciales
 * ante fallos de bus. La secuencia de adquisición corresponde a ImpedanceApp.
 */
class AD5934 {
 public:
  static constexpr uint8_t DEFAULT_ADDRESS = 0x0D; ///< Dirección I²C predeterminada de 7 bits.
  /** @brief Direcciones de los registros utilizados por el driver. */
  enum Register : uint8_t {
    CONTROL_HIGH = 0x80, CONTROL_LOW = 0x81,
    START_FREQUENCY_MSB = 0x82, START_FREQUENCY_MID = 0x83, START_FREQUENCY_LSB = 0x84,
    FREQUENCY_INCREMENT_MSB = 0x85, FREQUENCY_INCREMENT_MID = 0x86, FREQUENCY_INCREMENT_LSB = 0x87,
    NUMBER_INCREMENTS_MSB = 0x88, NUMBER_INCREMENTS_LSB = 0x89,
    SETTLING_CYCLES_MSB = 0x8A, SETTLING_CYCLES_LSB = 0x8B,
    STATUS = 0x8F, REAL_MSB = 0x94, REAL_LSB = 0x95,
    IMAGINARY_MSB = 0x96, IMAGINARY_LSB = 0x97
  };
  static constexpr uint8_t STATUS_DATA_VALID = 0x02; ///< Máscara D1: resultado DFT disponible.
  static constexpr uint8_t STATUS_SWEEP_COMPLETE = 0x04; ///< Máscara D2: barrido completo.
  static constexpr uint32_t MAX_OUTPUT_FREQUENCY_HZ = AD5934Math::MAX_OUTPUT_FREQUENCY_HZ; ///< Límite de frecuencia, en Hz.
  /** @brief Comandos de control soportados por sendCommand(). */
  enum class ControlCommand : uint8_t { InitializeStartFrequency = 0x1,
                                         StartFrequencySweep = 0x2,
                                         IncrementFrequency = 0x3 };
  /** @brief Grupo de registros contiguos utilizado para validar accesos. */
  struct RegisterGroup {
    uint8_t first; ///< Primera dirección del grupo.
    uint8_t size; ///< Tamaño del grupo, en bytes.
    const char* name; ///< Nombre estático del grupo.
    bool writable; ///< Indica si el grupo admite escritura según su tipo.
  };
  /** @brief Códigos de error de validación, control y comunicación I²C. */
  enum class ErrorCode : uint8_t {
    None, InvalidAddress, InvalidRegister, InvalidSpan, ReadOnly,
    ControlProtected, InvalidValue, TxBuffer, AddressNack, DataNack,
    Timeout, Bus, ShortRead
  };
  /** @brief Fase de la operación donde se registró un error. */
  enum class Phase : uint8_t { Validation, Probe, Pointer, Receive, Write };
  /** @brief Detalle del último error y progreso de una operación. */
  struct Error {
    ErrorCode code = ErrorCode::None; ///< Código del error.
    Phase phase = Phase::Validation; ///< Fase donde se produjo.
    uint8_t reg = 0; ///< Registro asociado.
    uint8_t wireCode = 0; ///< Código Wire cuando lo proporciona endTransmission().
    uint8_t completedBytes = 0; ///< Bytes completados antes del fallo.
  };

  /** @brief Vincula el bus Wire y la dirección del AD5934.
   * @param wire Bus que debe permanecer válido durante la vida del driver.
   * @param address Dirección I²C de 7 bits del dispositivo.
   */
  explicit AD5934(TwoWire& wire, uint8_t address = DEFAULT_ADDRESS);
  /** @brief Sondea el dispositivo sin inicializar Wire ni escribir registros.
   * @return true si responde; false ante dirección o comunicación inválidas.
   */
  bool begin();
  /** @brief Lee un byte de registro.
   * @param reg Dirección del registro.
   * @param[out] value Valor leído; no cambia si falla.
   * @return true si se leyó; false ante error de validación o I²C.
   */
  bool readRegister8(uint8_t reg, uint8_t& value);
  /** @brief Escribe un byte de configuración.
   * @param reg Dirección del registro.
   * @param value Valor que se va a escribir.
   * @return true si se escribió; false ante error de validación o I²C.
   * @note Los registros de control están protegidos frente a escrituras genéricas.
   */
  bool writeRegister8(uint8_t reg, uint8_t value);
  /** @brief Lee dos bytes contiguos de un grupo, MSB primero.
   * @param reg Primera dirección.
   * @param[out] value Valor leído; no cambia si falla.
   * @return true si se leyó el grupo; false en caso de error.
   */
  bool readRegister16(uint8_t reg, uint16_t& value);
  /** @brief Escribe dos bytes contiguos de un grupo, MSB primero.
   * @param reg Primera dirección.
   * @param value Valor que se va a escribir.
   * @return true si se escribieron ambos bytes; false en caso de error.
   * @note Un fallo I²C puede dejar una escritura parcial.
   */
  bool writeRegister16(uint8_t reg, uint16_t value);
  /** @brief Lee tres bytes contiguos de un grupo, MSB primero.
   * @param reg Primera dirección.
   * @param[out] value Valor leído; no cambia si falla.
   * @return true si se leyó el grupo; false en caso de error.
   */
  bool readRegister24(uint8_t reg, uint32_t& value);
  /** @brief Escribe tres bytes contiguos de un grupo, MSB primero.
   * @param reg Primera dirección.
   * @param value Valor de hasta 24 bits.
   * @return true si se escribieron todos los bytes; false en caso de error.
   * @note Un fallo I²C puede dejar una escritura parcial.
   */
  bool writeRegister24(uint8_t reg, uint32_t value);
  /** @brief Lee el registro STATUS.
   * @param[out] value Byte leído si la operación tiene éxito.
   * @return true si se leyó; false en caso de error.
   */
  bool readStatus(uint8_t& value);
  /** @brief Programa frecuencia inicial, incremento y número de incrementos cero, y settling.
   * @param startFrequencyCode Palabra DDS inicial de hasta 24 bits.
   * @param settlingWord Palabra de registro con ciclos y multiplicador permitido.
   * @return true si se escribieron los campos; false ante validación o error I²C.
   * @note Un fallo durante la escritura puede dejar campos parcialmente actualizados.
   */
  bool configureSweep(uint32_t startFrequencyCode, uint16_t settlingWord);
  /** @brief Escribe la palabra DDS de frecuencia inicial.
   * @param code Valor de hasta 24 bits.
   * @return true si se escribió; false en caso de error.
   */
  bool setStartFrequencyCode(uint32_t code);
  /** @brief Escribe la palabra DDS de incremento.
   * @param code Valor de hasta 24 bits.
   * @return true si se escribió; false en caso de error.
   */
  bool setFrequencyIncrementCode(uint32_t code);
  /** @brief Escribe el número de incrementos.
   * @param count Número entre 0 y 511.
   * @return true si se escribió; false en caso de error.
   */
  bool setNumberOfIncrements(uint16_t count);
  /** @brief Escribe ciclos de asentamiento con multiplicador por defecto.
   * @param cycles Número entre 0 y 511.
   * @return true si se escribió; false en caso de error.
   */
  bool setSettlingCycles(uint16_t cycles);
  /** @brief Configura Range 1 y PGA ×1 en power-down y verifica el control.
   * @return true si se configuró y verificó; false ante estado inválido o error I²C.
   */
  bool configureRange1PgaX1();
  /** @brief Lleva el dispositivo de power-down a standby, o verifica standby existente.
   * @param[out] previousControl Control leído antes de la operación.
   * @param[out] verifiedControl Control confirmado después de la operación.
   * @return true si queda en standby verificado; false en caso de error.
   */
  bool wakeToStandby(uint16_t& previousControl, uint16_t& verifiedControl);
  /** @brief Envía un comando de control permitido y verifica el registro.
   * @param command Operación Initialize, Start o Increment.
   * @return true si se envió y verificó; false ante transición inválida o error I²C.
   * @note Increment requiere STATUS_DATA_VALID; el rango y PGA se conservan.
   */
  bool sendCommand(ControlCommand command);
  /** @brief Lee la componente real DFT con signo.
   * @param[out] value Valor leído si la operación tiene éxito.
   * @return true si se leyó; false en caso de error.
   */
  bool readReal(int16_t& value);
  /** @brief Lee la componente imaginaria DFT con signo.
   * @param[out] value Valor leído si la operación tiene éxito.
   * @return true si se leyó; false en caso de error.
   */
  bool readImaginary(int16_t& value);
  /** @brief Calcula la palabra DDS de una frecuencia válida.
   * @param frequencyHz Frecuencia solicitada, en Hz.
   * @param mclkHz Frecuencia del reloj configurado, en Hz.
   * @param[out] code Palabra DDS calculada si la entrada es válida.
   * @return true si se puede representar; false si excede los límites o es inválida.
   */
  static bool frequencyToCode(uint32_t frequencyHz, uint32_t mclkHz, uint32_t& code);
  /** @brief Calcula la magnitud de una muestra DFT compleja.
   * @param real Componente real con signo.
   * @param imaginary Componente imaginaria con signo.
   * @return Magnitud DFT en unidades de código.
   */
  static double magnitude(int16_t real, int16_t imaginary);
  /** @brief Establece o verifica un estado inactivo seguro, conservando rango y PGA.
   * @param[out] previousControl Control leído antes de la operación.
   * @param[out] verifiedControl Control confirmado después de la operación.
   * @return true si el estado inactivo queda verificado; false en caso de error.
   * @note Acepta power-down seguro; wakeToStandby() establece standby cuando se requiere.
   */
  bool enterStandby(uint16_t& previousControl, uint16_t& verifiedControl);
  /** @brief Valida una escritura genérica sin enviarla por I²C.
   * @param reg Primera dirección de registro.
   * @param width Número de bytes, de 1 a 3.
   * @param value Valor que se pretende escribir.
   * @return true si el acceso es válido; false y lastError() actualizado en caso contrario.
   */
  bool validateWrite(uint8_t reg, uint8_t width, uint32_t value);
  /** @brief Valida una lectura genérica sin enviarla por I²C.
   * @param reg Primera dirección de registro.
   * @param width Número de bytes, de 1 a 3.
   * @return true si el acceso es válido; false y lastError() actualizado en caso contrario.
   */
  bool validateRead(uint8_t reg, uint8_t width);
  /** @brief Devuelve una referencia al detalle del último error del driver. */
  const Error& lastError() const { return error_; }
  /** @brief Devuelve la dirección I²C configurada. */
  uint8_t address() const { return address_; }
  /** @brief Busca el grupo que contiene un registro.
   * @param reg Dirección de registro.
   * @return Puntero al grupo estático o nullptr si no existe.
   */
  static const RegisterGroup* groupFor(uint8_t reg);
  /** @brief Consulta un grupo por su índice.
   * @param index Índice en el catálogo de grupos.
   * @return Puntero al grupo estático o nullptr si está fuera de rango.
   */
  static const RegisterGroup* groupAt(size_t index);
  /** @brief Obtiene la descripción de un código de error.
   * @param code Código a consultar.
   * @return Cadena estática descriptiva.
   */
  static const char* errorText(ErrorCode code);
  /** @brief Obtiene la descripción de una fase de operación.
   * @param phase Fase a consultar.
   * @return Cadena estática descriptiva.
   */
  static const char* phaseText(Phase phase);
  /** @brief Obtiene la máscara de bits significativos usada al validar escritura.
   * @param reg Dirección del registro.
   * @return Máscara de bits admitidos para el registro.
   */
  static uint8_t meaningfulMask(uint8_t reg);

 private:
  static constexpr uint8_t ADDRESS_POINTER = 0xB0;
  bool validateSpan(uint8_t reg, uint8_t width, bool writing);
  bool readBytes(uint8_t reg, uint8_t width, uint32_t& value);
  bool writeBytes(uint8_t reg, uint8_t width, uint32_t value);
  bool transmit(const uint8_t* bytes, size_t length, uint8_t reg, Phase phase);
  bool fail(ErrorCode code, uint8_t reg, Phase phase, uint8_t wireCode = 0);
  void clearError();
  TwoWire& wire_;
  uint8_t address_;
  Error error_;
};
