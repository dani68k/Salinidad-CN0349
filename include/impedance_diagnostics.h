#pragma once

#include <stdint.h>

namespace ImpedanceDiagnostics {
enum class ErrorCode : uint8_t {
  None, Busy, InvalidArgument, InvalidState, I2cNack, I2cTimeout, I2cError,
  RegisterVerifyFailed, Adg715VerifyFailed, DftTimeout, SweepTimeout,
  DataInvalid, RecoveryFailed, Count
};
enum class Subsystem : uint8_t { None, Application, Ad5934, Adg715, Bus };
enum class RecoveryState : uint8_t { NotAttempted, Pending, Complete, Partial, Failed };
struct ErrorRecord {
  ErrorCode code = ErrorCode::None;
  Subsystem subsystem = Subsystem::None;
  uint8_t phase = 0;
  uint32_t timestampMs = 0;
  const char* detail = "";
  bool recoverable = false;
  RecoveryState recovery = RecoveryState::NotAttempted;
};
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
