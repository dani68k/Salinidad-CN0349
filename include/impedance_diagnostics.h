#pragma once
/** @file impedance_diagnostics.h
 * @brief Tipos compartidos de diagnóstico y recuperación de impedancia.
 */

#include <stdint.h>

/** @brief Códigos, subsistemas y registros de diagnóstico de ImpedanceApp. */
namespace ImpedanceDiagnostics {
/** @brief Códigos de error funcional, I²C, verificación y adquisición.
 * Count es un centinela para dimensionar contadores, no un error operativo.
 */
enum class ErrorCode : uint8_t {
  None, Busy, InvalidArgument, InvalidState, I2cNack, I2cTimeout, I2cError,
  RegisterVerifyFailed, Adg715VerifyFailed, DftTimeout, SweepTimeout,
  DataInvalid, RecoveryFailed, Count
};
/** @brief Subsistema asociado al error. */
enum class Subsystem : uint8_t { None, Application, Ad5934, Adg715, Bus };
/** @brief Estado de recuperación asociado al error. */
enum class RecoveryState : uint8_t { NotAttempted, Pending, Complete, Partial, Failed };
/** @brief Registro de error y resultado de recuperación. */
struct ErrorRecord {
  ErrorCode code = ErrorCode::None; ///< Código de error.
  Subsystem subsystem = Subsystem::None; ///< Subsistema afectado.
  uint8_t phase = 0; ///< Fase de adquisición registrada.
  uint32_t timestampMs = 0; ///< Instante según millis(), en ms.
  const char* detail = ""; ///< Descripción textual referenciada por el registro.
  bool recoverable = false; ///< Indica si se considera recuperable.
  RecoveryState recovery = RecoveryState::NotAttempted; ///< Estado de recuperación.
};
/** @brief Obtiene el identificador textual de un error.
 * @param code Código a consultar.
 * @return Cadena estática o UNKNOWN para un código no reconocido o Count.
 */
inline const char* errorName(ErrorCode code) {
  switch (code) {
    case ErrorCode::None: return "NONE";
    case ErrorCode::Busy: return "BUSY";
    case ErrorCode::InvalidArgument: return "INVALID_ARGUMENT";
    case ErrorCode::InvalidState: return "INVALID_STATE";
    case ErrorCode::I2cNack: return "I2C_NACK";
    case ErrorCode::I2cTimeout: return "I2C_TIMEOUT";
    case ErrorCode::I2cError: return "I2C_ERROR";
    case ErrorCode::RegisterVerifyFailed: return "REGISTER_VERIFY_FAILED";
    case ErrorCode::Adg715VerifyFailed: return "ADG715_VERIFY_FAILED";
    case ErrorCode::DftTimeout: return "DFT_TIMEOUT";
    case ErrorCode::SweepTimeout: return "SWEEP_TIMEOUT";
    case ErrorCode::DataInvalid: return "DATA_INVALID";
    case ErrorCode::RecoveryFailed: return "RECOVERY_FAILED";
    case ErrorCode::Count: break;
  }
  return "UNKNOWN";
}
}  // namespace ImpedanceDiagnostics
