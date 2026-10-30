#pragma once
/** @file BleApp.h
 * @brief Servicio GATT ATLANTES e interfaz BLE del controlador CN0349.
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "../cn0349/CN0349Manager.h"

/** @brief Identificadores, tamaños y códigos del protocolo BLE ATLANTES.
 * @details Los paquetes binarios usan little-endian. El detalle de formatos,
 * offsets y propiedades de características se conserva en src/ble/README.md.
 */
namespace AtlantesBle {
constexpr char DEVICE_NAME[] = "CN0349"; ///< Nombre anunciado del dispositivo.
constexpr char SERVICE_UUID[] = "8d7b0001-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< UUID del servicio ATLANTES.
constexpr char STATUS_UUID[] = "8d7b0002-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Estado general, lectura y notificación.
constexpr char SETUP_UUID[] = "8d7b0003-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Setup básico aplicado, lectura y notificación.
constexpr char SETUP_COMMAND_UUID[] = "8d7b0004-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Orden de Setup básico, escritura.
constexpr char SETUP_RESULT_UUID[] = "8d7b0005-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Resultado de Setup, lectura y notificación.
constexpr char CALIBRATION_UUID[] = "8d7b0006-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Snapshot de calibraciones, lectura.
constexpr char MEASUREMENT_UUID[] = "8d7b0007-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Resultado de ciclo, lectura y notificación.
constexpr char CONTROL_COMMAND_UUID[] = "8d7b0008-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Orden de control, escritura.
constexpr char CONTROL_RESULT_UUID[] = "8d7b0009-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Resultado de control, lectura y notificación.
constexpr char ADVANCED_UUID[] = "8d7b000a-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Perfil AD5934 configurado, lectura y notificación.
constexpr char EC_DIAGNOSTIC_UUID[] = "8d7b000b-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Diagnóstico EC por ciclo, lectura y notificación.
constexpr char SETUP_EXT_UUID[] = "8d7b000c-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Setup con compensación, lectura y notificación.
constexpr char EC25_UUID[] = "8d7b000d-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< EC25 por ciclo, lectura y notificación.
constexpr char SETUP_EXT_COMMAND_UUID[] = "8d7b000e-6f7a-4f31-9c4d-2b6e6a7f3a10"; ///< Orden de Setup ampliado, escritura.
constexpr size_t SETUP_EXT_SIZE = 17; ///< Tamaño del Setup ampliado, en bytes.
constexpr size_t SETUP_EXT_COMMAND_SIZE = 18; ///< Tamaño de la orden de Setup ampliado, en bytes.
constexpr size_t EC25_SIZE = 11; ///< Tamaño del resultado EC25, en bytes.
constexpr size_t EC_DIAGNOSTIC_SIZE = 7; ///< Tamaño del diagnóstico EC, en bytes.
constexpr size_t ADVANCED_SIZE = 20; ///< Tamaño del perfil AD5934, en bytes.
constexpr size_t STATUS_CAPACITY = 80; ///< Capacidad del búfer STATUS, en bytes.
constexpr size_t SETUP_SIZE = 12; ///< Tamaño del Setup básico, en bytes.
constexpr size_t SETUP_COMMAND_SIZE = 13; ///< Tamaño de la orden de Setup básico, en bytes.
constexpr size_t SETUP_RESULT_SIZE = 3; ///< Tamaño del resultado de Setup, en bytes.
constexpr size_t CALIBRATION_SIZE = 76; ///< Tamaño del snapshot de calibración, en bytes.
constexpr size_t MEASUREMENT_SIZE = 26; ///< Tamaño del resultado de medida, en bytes.
constexpr size_t CONTROL_COMMAND_SIZE = 4; ///< Tamaño de la orden de control, en bytes.
constexpr size_t CONTROL_RESULT_SIZE = 4; ///< Tamaño del resultado de control, en bytes.
constexpr uint8_t PROTOCOL_VERSION = 1; ///< Versión de los paquetes binarios.

/** @brief Códigos de resultado de una orden de Setup. */
enum class SetupResult : uint8_t {
  Ok = 0, InvalidLength = 1, UnsupportedVersion = 2, InvalidValue = 3,
  NotReady = 4, Busy = 5, InternalError = 6
};
/** @brief Operaciones de calibración, selección de sensores y medición. */
enum class ControlOperation : uint8_t {
  CalRun = 1, SetSensors = 2, MeasureOnce = 3, MeasureStart = 4, MeasureStop = 5
};
/** @brief Códigos de resultado de una orden de control. */
enum class ControlResult : uint8_t {
  Ok = 0, InvalidLength = 1, UnsupportedVersion = 2, InvalidOperation = 3,
  NotReady = 4, Busy = 5, InvalidArgument = 6, Hardware = 7,
  NotCalibrated = 8, Timeout = 9, MeasurementFailed = 10, Cancelled = 11,
  InternalError = 12
};
}

/** @brief Periférico BLE que expone el controlador del instrumento.
 * @details Los callbacks encolan solicitudes; update() procesa las órdenes y
 * publica snapshots. Al desconectar el último central, solicita desde el loop
 * la parada de una medición activa y descarta órdenes de control pendientes.
 */
class BleApp {
 public:
  /** @brief Vincula el controlador compartido.
   * @param instrument CN0349Manager que debe permanecer válido durante la vida de BleApp.
   */
  explicit BleApp(CN0349Manager& instrument) : instrument_(instrument) {}
  /** @brief Inicializa NimBLE, crea el servicio y comienza el advertising.
   * @note Si ya se ha inicializado, no repite la inicialización. No devuelve un estado de error.
   */
  void begin();
  /** @brief Procesa solicitudes y actualiza las características GATT.
   * @details Llamar periódicamente desde el loop después del controlador.
   * Publica el último ciclo sin consumir el slot usado por Terminal.
   */
  void update();

 private:
  class ServerCallbacks : public NimBLEServerCallbacks {
   public:
    explicit ServerCallbacks(BleApp& owner) : owner_(owner) {}
    void onConnect(NimBLEServer*, NimBLEConnInfo&) override;
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override;
   private:
    BleApp& owner_;
  };

  class CommandCallbacks : public NimBLECharacteristicCallbacks {
   public:
    explicit CommandCallbacks(BleApp& owner) : owner_(owner) {}
    void onWrite(NimBLECharacteristic*, NimBLEConnInfo&) override;
   private:
    BleApp& owner_;
  };
  class ControlCallbacks : public NimBLECharacteristicCallbacks {
   public:
    explicit ControlCallbacks(BleApp& owner) : owner_(owner) {}
    void onWrite(NimBLECharacteristic*, NimBLEConnInfo&) override;
   private:
    BleApp& owner_;
  };

  static const char* stateName(const InstrumentStatus& status);
  void publishAdvanced();
  void publishStatus(bool forceNotify);
  void publishSetup(bool forceNotify);
  void publishSetupResult(uint8_t requestId, AtlantesBle::SetupResult result);
  void publishCalibration();
  void publishMeasurement();
  void publishControlResult(uint8_t requestId, uint8_t operation, AtlantesBle::ControlResult result);
  void enqueueSetupCommand(const uint8_t* data, size_t size);
  void processSetupCommand();
  void enqueueControlCommand(const uint8_t* data, size_t size);
  void processControlCommand();
  static AtlantesBle::ControlResult controlResult(InstrumentError error);
  static void encodeSetup(const InstrumentSetup& setup, uint8_t* output);
  static bool decodeSetup(const uint8_t* input, InstrumentSetup& setup);

  CN0349Manager& instrument_;
  ServerCallbacks callbacks_{*this};
  CommandCallbacks commandCallbacks_{*this};
  ControlCallbacks controlCallbacks_{*this};
  NimBLECharacteristic* advancedCharacteristic_ = nullptr;
  uint8_t lastAdvanced_[AtlantesBle::ADVANCED_SIZE]{};
  bool hasLastAdvanced_ = false;
  NimBLECharacteristic* statusCharacteristic_ = nullptr;
  NimBLECharacteristic* setupCharacteristic_ = nullptr;
  NimBLECharacteristic* setupExtCharacteristic_ = nullptr;
  NimBLECharacteristic* ec25Characteristic_ = nullptr;
  uint8_t lastSetupExt_[AtlantesBle::SETUP_EXT_SIZE]{};
  bool hasLastSetupExt_ = false;
  NimBLECharacteristic* setupResultCharacteristic_ = nullptr;
  NimBLECharacteristic* calibrationCharacteristic_ = nullptr;
  NimBLECharacteristic* measurementCharacteristic_ = nullptr;
  NimBLECharacteristic* ecDiagnosticCharacteristic_ = nullptr;
  NimBLECharacteristic* controlResultCharacteristic_ = nullptr;
  bool initialized_ = false;
  volatile bool connected_ = false;
  volatile bool commandPending_ = false;
  volatile bool rejectedPending_ = false;
  volatile uint8_t rejectedRequestId_ = 0;
  portMUX_TYPE commandMux_ = portMUX_INITIALIZER_UNLOCKED;
  uint8_t command_[AtlantesBle::SETUP_EXT_COMMAND_SIZE]{};
  size_t commandSize_ = 0;
  char lastStatus_[AtlantesBle::STATUS_CAPACITY]{};
  uint8_t lastSetup_[AtlantesBle::SETUP_SIZE]{};
  bool hasLastSetup_ = false;
  uint8_t lastCalibration_[AtlantesBle::CALIBRATION_SIZE]{};
  bool hasLastCalibration_ = false;
  portMUX_TYPE controlMux_ = portMUX_INITIALIZER_UNLOCKED;
  bool disconnectStopPending_ = false;
  volatile bool controlPending_ = false;
  volatile bool controlRejectedPending_ = false;
  volatile uint8_t controlRejectedRequestId_ = 0;
  volatile uint8_t controlRejectedOperation_ = 0;
  uint8_t controlCommand_[AtlantesBle::CONTROL_COMMAND_SIZE]{};
  size_t controlCommandSize_ = 0;
  uint32_t lastPublishedCycle_ = 0;
};
