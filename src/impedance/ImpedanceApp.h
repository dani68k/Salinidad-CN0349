#pragma once
/** @file ImpedanceApp.h
 * @brief Adquisición DFT cooperativa y recuperación de AD5934 y ADG715.
 */
#include <AD5934.h>
#include "impedance_diagnostics.h"
#include "../mux/MuxApp.h"
/** @brief Coordina una adquisición DFT única mediante AD5934 y MuxApp.
 * @details Las peticiones se completan mediante update(); el controlador
 * superior secuencia sensores y ciclos. No convierte temperatura ni EC.
 */
class ImpedanceApp {
 public:
  /** @brief Perfil configurado para adquisición; no es un readback del hardware. */
  struct Profile {
    uint32_t mclkHz = 1000000; ///< Reloj configurado, en Hz.
    uint32_t frequencyHz = 2000; ///< Frecuencia solicitada, en Hz.
    uint16_t settlingCycles = 10; ///< Ciclos de asentamiento del AD5934.
    uint16_t startSettlingCycles = 10; ///< Ciclos de espera tras Initialize.
    uint16_t numberOfIncrements = 0; ///< Número de incrementos configurado.
    uint32_t frequencyIncrementHz = 0; ///< Incremento configurado, en Hz.
    MuxApp::CalibrationPreset calibrationPreset = MuxApp::CalibrationPreset::CalLow1K; ///< Referencia para requestSingle().
    uint8_t excitationRange = 1; ///< Rango de salida configurado.
    uint8_t pgaGain = 1; ///< Ganancia PGA configurada.
    bool externalClock = true; ///< Indica el uso previsto de reloj externo.
  };
  /** @brief Fases de la adquisición, parada y estados finales. */
  enum class MeasurementState : uint8_t {
    Idle, Configuring, PreparingStandby, Initializing, Settling, Acquiring,
    ReadingReal, ReadingImaginary, Stopping, Complete, Error, Cancelled
  };
  /** @brief Resultado DFT de la adquisición completada. */
  struct Result {
    int16_t real = 0; ///< Componente real con signo.
    int16_t imaginary = 0; ///< Componente imaginaria con signo.
    double magnitude = 0; ///< Magnitud DFT.
    uint32_t frequencyHz = 0; ///< Frecuencia solicitada, en Hz.
    double effectiveFrequencyHz = 0; ///< Frecuencia DDS efectiva, en Hz.
    uint32_t elapsedMs = 0; ///< Duración de la operación, en ms.
    MuxApp::CalibrationInfo calibration{}; ///< Información de la ruta utilizada.
    double phaseDegrees = 0; ///< Fase DFT cruda, en grados.
    bool valid = false; ///< Indica resultado completado válido.
  };
  /** @brief Snapshot de comunicación, readiness y errores de adquisición. */
  struct ImpedanceStatus {
    bool busy = false; ///< La máquina de adquisición está activa.
    bool ad5934Communication = false; ///< Comunicación AD5934 observada.
    bool adg715Communication = false; ///< Comunicación ADG715 observada.
    bool configurationVerified = false; ///< Configuración comprobada durante diagnóstico.
    bool operational = false; ///< Estado operativo conocido.
    bool diagnosticComplete = false; ///< Diagnóstico de hardware completado.
    bool hardwareReady = false; ///< Se cumplen los requisitos de readiness.
    bool lastOperationSuccessful = false; ///< La última operación terminó correctamente.
    bool adg715MaskKnown = false; ///< Existe máscara observada del ADG715.
    uint8_t adg715Mask = 0; ///< Última máscara observada del ADG715.
    uint32_t elapsedMs = 0; ///< Duración activa o de la última operación, en ms.
    ImpedanceDiagnostics::ErrorCode activeError = ImpedanceDiagnostics::ErrorCode::None; ///< Error activo.
    ImpedanceDiagnostics::ErrorCode lastError = ImpedanceDiagnostics::ErrorCode::None; ///< Último error registrado.
  };
  /** @brief Vincula los drivers de impedancia y conmutación.
   * @param driver Instancia AD5934 que debe permanecer válida.
   * @param mux Instancia MuxApp que debe permanecer válida.
   */
  ImpedanceApp(AD5934& driver, MuxApp& mux);
  /** @brief Avanza la adquisición activa y actualiza el estado público. */
  void update();
  /** @brief Devuelve true mientras la máquina no está en un estado final o Idle. */
  bool isBusy() const;
  /** @brief Devuelve la fase actual de adquisición. */
  MeasurementState measurementState() const { return measurementState_; }
  /** @brief Devuelve una referencia al perfil configurado, no a registros leídos. */
  const Profile& profile() const { return profile_; }
  /** @brief Devuelve una copia del estado público sin realizar transacciones I²C. */
  ImpedanceStatus statusSnapshot() const;
  /** @brief Copia el resultado de una adquisición completada.
   * @param[out] result Resultado DFT si está disponible.
   * @return true en Complete con resultado; false en otro caso.
   */
  bool singleResult(Result& result) const {
    if (!hasResult_ || measurementState_ != MeasurementState::Complete) return false;
    result = result_; return true;
  }
  /** @brief Configura referencia, frecuencia y settling para una adquisición.
   * @param preset Referencia electrónica autorizada.
   * @param frequencyHz Frecuencia solicitada, en Hz.
   * @param settlingCycles Ciclos de asentamiento, entre 0 y 511.
   * @return true si el perfil es válido y se ha configurado; false si está ocupado o es inválido.
   * @note Configura el perfil en RAM; los registros se escriben durante la adquisición.
   */
  bool configureSingle(MuxApp::CalibrationPreset preset, uint32_t frequencyHz, uint16_t settlingCycles);
  /** @brief Solicita una adquisición de la referencia del perfil.
   * @param busReady Estado de inicialización del bus I²C.
   * @return true si se inició; false si no puede iniciarse.
   */
  bool requestSingle(bool busReady) { return startMeasurement(busReady, 0); }
  /** @brief Solicita una adquisición por la ruta Pt100.
   * @param busReady Estado de inicialización del bus I²C.
   * @return true si se inició; false si no puede iniciarse.
   */
  bool requestPt100(bool busReady) { return startMeasurement(busReady, 0x41); }
  /** @brief Solicita una adquisición por una ruta EC.
   * @param busReady Estado de inicialización del bus I²C.
   * @param mask Máscara de ruta EC: 0x81 para HIGH o 0x82 para LOW.
   * @return true si se inició; false si no puede iniciarse.
   */
  bool requestEc(bool busReady, uint8_t mask) { return startMeasurement(busReady, mask); }
  /** @brief Consulta los datos de una referencia electrónica.
   * @param p Referencia solicitada.
   * @param[out] info Información de la ruta si existe.
   * @return true si se encontró la referencia; false en caso contrario.
   */
  bool presetInfo(MuxApp::CalibrationPreset p, MuxApp::CalibrationInfo& info) const { return mux_.resolveCalibrationPreset(p, info); }
  /** @brief Solicita cancelar una operación activa y pasar por parada segura.
   * @return true si se solicitó; false si no había operación activa.
   */
  bool cancelOperation();
  /** @brief Recupera y comprueba el hardware cuando no hay adquisición activa.
   * @param busReady Estado de inicialización del bus I²C.
   * @return true si el hardware queda listo; false si está ocupado o falla la recuperación.
   */
  bool requestRecovery(bool busReady);
 private:
  bool startMeasurement(bool busReady, uint8_t routeMask);
  void advanceMeasurement();
  void failMeasurement(const char* message, bool recover);
  void finishStopping();
  void setMeasurementState(MeasurementState state);
  void recover();
  void recordError(ImpedanceDiagnostics::ErrorCode code, ImpedanceDiagnostics::Subsystem subsystem,
                   const char* detail, bool recoverable);
  void refreshPublicStatus();
  AD5934& driver_;
  MuxApp& mux_;
  Profile profile_;
  MeasurementState measurementState_ = MeasurementState::Idle;
  Result result_;
  bool hasResult_ = false;
  uint32_t activeStartHz_ = 0;
  uint16_t activeIncrements_ = 0;
  MuxApp::CalibrationInfo measurementCalibration_{};
  bool cancelled_ = false;
  uint8_t configPhase_ = 0;
  uint8_t verifyPhase_ = 0;
  uint32_t startCode_ = 0;
  uint32_t incrementCode_ = 0;
  uint32_t measurementStartedMs_ = 0;
  uint32_t acquisitionStartedMs_ = 0;
  uint32_t stateStartedMs_ = 0;
  uint32_t lastPollMs_ = 0;
  uint32_t measurementElapsedMs_ = 0;
  int16_t pendingReal_ = 0;
  int16_t pendingImaginary_ = 0;
  const char* measurementError_ = nullptr;
  AD5934::Error driverError_;
  ImpedanceStatus publicStatus_{};
  ImpedanceDiagnostics::ErrorRecord lastErrorRecord_{};
  uint32_t errorCounters_[static_cast<uint8_t>(ImpedanceDiagnostics::ErrorCode::Count)]{};
};
