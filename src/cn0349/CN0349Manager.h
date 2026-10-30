#pragma once
/**
 * @file CN0349Manager.h
 * @brief API de configuración, calibración y medición del instrumento CN0349.
 *
 * Las operaciones se coordinan desde el loop mediante CN0349Manager::update().
 * Las interfaces de terminal y BLE utilizan el mismo controlador.
 */
#include "../impedance/ImpedanceApp.h"
#include "../temperature/Pt100.h"
#include "../conductivity/Conductivity.h"

/** @brief Referencia electrónica seleccionada para calibración de impedancia. */
enum class CN0349Preset : uint8_t { CalHigh100R, CalHigh1K, CalLow1K, CalLow10K };
/** @brief Resultado de una operación del controlador.
 * Persistence indica que el cambio está en RAM, pero falló su guardado en NVS.
 */
enum class InstrumentError : uint8_t { None, Busy, InvalidArgument, Hardware, NotCalibrated,
  Timeout, MeasurementFailed, Cancelled, Persistence };
/** @brief Estado general del instrumento. */
enum class InstrumentState : uint8_t { NotInitialized, Calibrating, Idle, Measuring, Stopping, Error };
/** @brief Estado de una referencia de calibración. */
enum class CalibrationState : uint8_t { Invalid, Running, Valid };
/** @brief Ruta de medida de conductividad. */
enum class CN0349EcRange : uint8_t { High, Low };
/** @brief Selección de sensores; Both incluye Pt100 y EC. */
enum class MeasurementSensors : uint8_t { Pt100 = 1, Ec = 2, Both = 3 };
/** @brief Diagnóstico de magnitud DFT EC frente al intervalo calibrado.
 * NearLimit corresponde al último 15 % junto al máximo. AboveRange no
 * significa que se haya detectado saturación física del ADC.
 */
enum class EcRangeDiagnostic : uint8_t { InvalidCal, InRange, NearLimit, BelowRange, AboveRange };
/** @brief Número de muestras por referencia electrónica. */
static constexpr uint8_t CN0349_CALIBRATION_SAMPLES = 5;
/** @brief Resultado y metadatos de una referencia de calibración electrónica. */
struct CN0349Calibration {
  CN0349Preset preset = CN0349Preset::CalHigh100R; ///< Referencia electrónica.
  CalibrationState state = CalibrationState::Invalid; ///< Estado de calibración.
  InstrumentError error = InstrumentError::NotCalibrated; ///< Error funcional.
  ImpedanceDiagnostics::ErrorCode hardwareError = ImpedanceDiagnostics::ErrorCode::None; ///< Error de adquisición.
  uint8_t presetMask = 0; ///< Máscara ADG715 de la referencia.
  float frequencyHz = 0; ///< Frecuencia solicitada, en Hz.
  float effectiveFrequencyHz = 0; ///< Frecuencia DDS efectiva, en Hz.
  float mclkHz = 0; ///< Reloj configurado, en Hz.
  float rfbOhms = 0; ///< Resistencia nominal de realimentación, en Ω.
  float rcalOhms = 0; ///< Resistencia nominal de referencia, en Ω.
  uint8_t excitationRange = 0; ///< Rango de excitación configurado.
  uint8_t pgaGain = 0; ///< Ganancia PGA configurada.
  uint16_t settlingCycles = 0; ///< Ciclos de asentamiento configurados.
  uint8_t sampleCount = 0; ///< Número de muestras válidas acumuladas.
  int16_t real[CN0349_CALIBRATION_SAMPLES]{}; ///< Muestras DFT reales.
  int16_t imag[CN0349_CALIBRATION_SAMPLES]{}; ///< Muestras DFT imaginarias.
  float sampleMagnitude[CN0349_CALIBRATION_SAMPLES]{}; ///< Magnitudes DFT por muestra.
  float samplePhaseDeg[CN0349_CALIBRATION_SAMPLES]{}; ///< Fases DFT por muestra, en grados.
  int16_t representativeReal = 0; ///< Componente real representativa.
  int16_t representativeImag = 0; ///< Componente imaginaria representativa.
  float meanMagnitude = 0; ///< Magnitud DFT media.
  float magnitudeStdDev = 0; ///< Desviación muestral de magnitud.
  float relativeVariationPct = 0; ///< Variación relativa, en porcentaje.
  float meanPhaseDeg = 0; ///< Fase media circular, en grados.
  float gainFactorPerOhm = 0; ///< Factor legacy, en 1/(Ω · código DFT).
  uint32_t timestampMs = 0; ///< Finalización según millis(), en ms.
  uint32_t durationMs = 0; ///< Duración de la referencia, en ms.
  uint32_t calibrationId = 0; ///< Identificador de la referencia.
  bool valid = false; ///< La referencia se completó y es válida.
  bool compatibleWithCurrentConfig = false; ///< Compatibilidad con el perfil actual.
};

/** @brief Estado de adquisición de impedancia de un sensor. */
enum class CN0349ImpedanceState : uint8_t { Idle, Running, Complete, Failed, Cancelled };
/** @brief Adquisición DFT y metadatos de impedancia del sensor. */
struct CN0349ImpedanceResult {
  bool valid = false; ///< Indica adquisición y procesamiento de impedancia válidos.
  uint8_t measurementMask = 0x41; ///< Máscara de medida ADG715.
  uint8_t calibrationMask = 0; ///< Máscara de referencia ADG715.
  CN0349Preset calibrationPreset = CN0349Preset::CalHigh100R; ///< Referencia utilizada.
  uint32_t calibrationId = 0; ///< Identificador de calibración.
  float frequencyHz = 2000; ///< Frecuencia solicitada, en Hz.
  float effectiveFrequencyHz = 0; ///< Frecuencia DDS de medida, en Hz.
  float calibrationEffectiveFrequencyHz = 0; ///< Frecuencia DDS de calibración, en Hz.
  uint32_t mclkHz = 0; ///< Reloj configurado, en Hz.
  uint8_t excitationRange = 0; ///< Rango de excitación.
  uint8_t pgaGain = 0; ///< Ganancia PGA.
  uint16_t settlingCycles = 0; ///< Ciclos de asentamiento.
  int16_t real = 0; ///< Componente DFT real con signo.
  int16_t imag = 0; ///< Componente DFT imaginaria con signo.
  float magnitude = 0; ///< Magnitud DFT.
  float rawPhaseDeg = 0; ///< Fase DFT, en grados.
  float calibrationPhaseDeg = 0; ///< Fase de referencia, en grados.
  float correctedPhaseDeg = 0; ///< Fase corregida instrumentalmente, en grados.
  float gainFactor = 0; ///< Factor legacy de calibración.
  float rfbOhms = 0, rcalOhms = 0; ///< Resistencias nominales de realimentación y referencia, en Ω.
  float apparentImpedanceOhms = 0; ///< Impedancia aparente, en Ω.
  uint32_t elapsedMs = 0; ///< Duración de la adquisición, en ms.
  CN0349ImpedanceState state = CN0349ImpedanceState::Idle; ///< Estado de adquisición.
  InstrumentError error = InstrumentError::None; ///< Error funcional.
  ImpedanceDiagnostics::ErrorCode hardwareError = ImpedanceDiagnostics::ErrorCode::None; ///< Error de hardware.
};


/** @brief Configuración efectiva utilizada para nuevos ciclos.
 * La compensación activa exige ambos sensores; alpha admite 0–5 %/°C con
 * dos decimales. La configuración se persiste, pero las calibraciones no.
 */
struct InstrumentSetup {
  MeasurementSensors sensors = MeasurementSensors::Both; ///< Sensores seleccionados.
  uint32_t intervalMs = 1000; ///< Periodo de inicio entre ciclos, en ms.
  CN0349EcRange ecRange = CN0349EcRange::Low; ///< Ruta EC.
  CN0349Preset ecCalibration = CN0349Preset::CalLow10K; ///< Referencia EC elegida.
  Conductivity::Model ecModel = Conductivity::Model::Raw; ///< Modelo de conversión EC.
  float nominalKPerCm = 1.0f; ///< Constante nominal de celda, en cm⁻¹.
  bool temperatureCompensation = false; ///< Activa el cálculo EC25.
  float temperatureAlpha = 2.0f; ///< Coeficiente lineal, en %/°C.
};
/** @brief Resultado individual de Pt100 o EC dentro de un ciclo. */
struct SensorMeasurement {
  MeasurementSensors sensor = MeasurementSensors::Pt100; ///< Sensor correspondiente.
  EcRangeDiagnostic ecRangeDiagnostic = EcRangeDiagnostic::InvalidCal; ///< Diagnóstico de rango EC.
  CN0349ImpedanceResult acquisition{}; ///< Datos y estado de adquisición.
  Pt100::Result temperature{}; ///< Conversión de temperatura Pt100.
  Conductivity::Result conductivity{}; ///< Conversión de conductividad EC.
  InstrumentError error = InstrumentError::MeasurementFailed; ///< Error del sensor.
  bool valid = false; ///< Indica que el resultado del sensor es válido.
};
/** @brief Snapshot de un ciclo con Setup, lecturas y estado congelados.
 * ec25MicroSiemens solo representa un resultado disponible si ec25Valid es true.
 */
struct MeasurementCycle {
  uint32_t cycleId = 0; ///< Identificador incremental desde el arranque.
  uint32_t startedMs = 0, finishedMs = 0; ///< Inicio y fin según millis(), en ms.
  InstrumentSetup setup{}; ///< Configuración congelada al iniciar el ciclo.
  SensorMeasurement pt100{}, ec{}; ///< Resultados de Pt100 y EC del ciclo.
  bool valid = false; ///< Todos los sensores seleccionados son válidos.
  bool cancelled = false; ///< Ciclo cancelado antes de completar.
  bool intervalExceeded = false; ///< La duración superó el periodo configurado en continuo.
  bool ec25Valid = false; ///< Indica disponibilidad de EC25.
  float ec25MicroSiemens = 0; ///< Conductividad EC25, en µS/cm; consultar ec25Valid.
  InstrumentError error = InstrumentError::None; ///< Error general del ciclo.
};
/** @brief Estado y contadores observados del instrumento. */
struct InstrumentStatus {
  InstrumentState state = InstrumentState::NotInitialized; ///< Estado general.
  bool hardwareReady = false, continuous = false; ///< Disponibilidad de hardware y modo continuo.
  uint8_t validCalibrations = 0; ///< Número de referencias legacy compatibles.
  uint32_t calibrationRunId = 0; ///< Identificador de sesión de calibración.
  uint32_t intervalExceededCount = 0; ///< Ciclos que excedieron el periodo.
  uint32_t publishedCycles = 0; ///< Ciclos publicados desde el arranque.
  uint32_t overwrittenResults = 0; ///< Publicaciones que sustituyeron un resultado no consumido.
  InstrumentError lastError = InstrumentError::None; ///< Último error del instrumento.
  bool adLowValid = false, adHighValid = false; ///< Disponibilidad de calibración AD por rango.
};

/**
 * @brief API central del instrumento, compartida por Terminal y BLE.
 * @details Coordina una única adquisición, calibración, Setup persistente y
 * ciclos Pt100/EC. Se utiliza desde el contexto cooperativo del loop; no
 * proporciona sincronización para llamadas concurrentes.
 */
class CN0349Manager {
 public:
  /** @brief Vincula el controlador a la adquisición compartida.
   * @param acquisition Adquisición que debe permanecer válida durante la vida del controlador.
   */
  explicit CN0349Manager(ImpedanceApp& acquisition) : acquisition_(acquisition) {}
  /** @brief Carga Setup, recupera hardware y arranca la calibración.
   * @param busReady Resultado de inicializar el bus I²C.
   * @return None si comienza correctamente, Busy si ya se inició o Hardware si falla la recuperación.
   */
  InstrumentError begin(bool busReady);
  /** @brief Avanza cooperativamente las operaciones pendientes. */
  void update();
  /** @brief Devuelve una copia del estado, calibraciones y contadores. */
  InstrumentStatus getStatus() const;
  /** @brief Devuelve una copia del Setup aplicado en RAM. */
  InstrumentSetup getSetup() const { return setup_; }
  /** @brief Devuelve el perfil configurado, sin leer registros de hardware. */
  ImpedanceApp::Profile getImpedanceProfile() const { return acquisition_.profile(); }
  /** @brief Valida y aplica el Setup completo; persiste cambios efectivos.
   * @param setup Valores solicitados, incluida selección de sensores.
   * @return None, Busy o InvalidArgument; Persistence si se aplicó en RAM pero no se guardó.
   */
  InstrumentError setSetup(const InstrumentSetup& setup);
  /** @brief Actualiza sensores mediante la misma validación y persistencia de Setup.
   * @param sensors Pt100, EC o ambos.
   * @return Resultado de setSetup().
   */
  InstrumentError setMeasurementSensors(MeasurementSensors sensors);
  /** @brief Solicita una calibración electrónica de las cuatro referencias.
   * @return None si se acepta; Busy o Hardware cuando no puede iniciarse.
   * @note Si hay continuo activo, solicita su parada antes de calibrar.
   */
  InstrumentError runCalibration();
  /** @brief Consulta una referencia legacy, incluso si no es válida.
   * @param preset Referencia solicitada.
   * @param[out] result Copia de la referencia y su compatibilidad actual.
   * @return false si el preset no existe; true si se ha copiado.
   */
  bool getCalibrationStatus(CN0349Preset preset, CN0349Calibration& result) const;
  /** @brief Consulta la calibración AD de un rango EC.
   * @param range Rango LOW o HIGH.
   * @param[out] result Copia de la calibración, cuya validez debe comprobarse.
   * @return false si el rango no existe; true si se ha copiado.
   */
  bool getAdCalibrationStatus(CN0349EcRange range, Conductivity::AdCalibration& result) const;
  /** @brief Inicia un ciclo de medida si se cumplen las condiciones previas.
   * @return None si se inicia; de lo contrario, error de ocupación, hardware o calibración.
   */
  InstrumentError startSingleMeasurement();
  /** @brief Inicia ciclos periódicos, comenzando por un ciclo inmediato.
   * @return None si se inicia; de lo contrario, error de ocupación, hardware o calibración.
   */
  InstrumentError startContinuousMeasurement();
  /** @brief Solicita la parada y la cancelación segura de la medida activa.
   * @return None si se acepta o Busy durante calibración.
   * @note La parada se completa en llamadas posteriores a update().
   */
  InstrumentError stopMeasurement();
  /** @brief Indica si hay adquisición, calibración o modo continuo activo. */
  bool isBusy() const;
  /** @brief Copia el último ciclo publicado sin consumirlo.
   * @param[out] result Snapshot del ciclo, si está disponible.
   * @return true si existe un resultado; false en caso contrario.
   */
  bool getMeasurementResult(MeasurementCycle& result) const;
  /** @brief Copia y consume el resultado pendiente del slot compartido.
   * @param[out] result Snapshot del ciclo pendiente.
   * @return true si se consumió un resultado; false si no había ninguno.
   */
  bool takeMeasurementResult(MeasurementCycle& result);
 private:
  static bool validSetup(const InstrumentSetup& setup);
  void loadSetup();
  bool saveSetup() const;
  bool compatible(const CN0349Calibration& c, float rfb) const;
  EcRangeDiagnostic diagnoseEcRange(CN0349EcRange range, float magnitude, float effectiveFrequencyHz) const;
  bool compatible(const Conductivity::AdCalibration& c, CN0349EcRange range) const;
  void buildAdCalibrations();
  InstrumentError preflight() const;
  void beginCalibration();
  void startCalibrationSlot();
  void updateCalibration();
  void completeCalibrationSlot(InstrumentError error);
  void beginCycle();
  void startSensor(MeasurementSensors sensor);
  void finishSensor();
  void finishCycle(bool cancelled = false);
  InstrumentError acquisitionError() const;
  ImpedanceApp& acquisition_;
  bool initialized_ = false, busReady_ = false;
  bool continuous_ = false, cycleActive_ = false, sensorStarted_ = false;
  bool calibrationPending_ = false;
  InstrumentState state_ = InstrumentState::NotInitialized;
  InstrumentError lastError_ = InstrumentError::None;
  InstrumentSetup setup_{};
  bool setupPersistenceFailed_ = false;
  CN0349Calibration calibrations_[4]{};
  Conductivity::AdCalibration adCalibrations_[2]{};
  uint8_t calibrationSlot_ = 0;
  bool calibrationSampleStarted_ = false;
  uint32_t calibrationSlotStartedMs_ = 0, nextCalibrationId_ = 1, calibrationRunId_ = 0;
  MeasurementSensors activeSensor_ = MeasurementSensors::Pt100;
  MeasurementCycle cycle_{}, result_{};
  bool hasResult_ = false, resultPending_ = false;
  uint32_t nextCycleId_ = 1, lastCycleStartedMs_ = 0, sensorStartedMs_ = 0;
  uint32_t intervalExceededCount_ = 0, publishedCycles_ = 0, overwrittenResults_ = 0;
};
