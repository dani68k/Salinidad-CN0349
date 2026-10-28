# CN0349Manager — controlador del instrumento

## Hito 2G

`buildAdCalibrations` crea LOW/HIGH al terminar los cuatro slots existentes.
La API añade `getAdCalibrationStatus`; preflight exige AD solo para ese modelo.
RAW/SERIES conservan impedancia legacy y AD procesa magnitud DFT directa. Cada
ciclo congela setup, ruta, ID y coeficientes. Sin autorange, compensación a
25 °C, NVS ni BLE.

Hito 2F, 2026-10-27. Propósito: coordinar setup, calibración electrónica y ciclos
Pt100/EC usando la adquisición existente, sin dependencia de Serial/Terminal.
Se simplifica esta clase en lugar de introducir InstrumentController adicional.

Interfaz: `begin`, `update`, `getStatus`, `getSetup`, `setSetup`, `runCalibration`,
`getCalibrationStatus`, `setMeasurementSensors`, `startSingleMeasurement`,
`startContinuousMeasurement`, `stopMeasurement`, `getMeasurementResult` y
`takeMeasurementResult`. Todos los setters/inicios devuelven InstrumentError.
Consultas devuelven copias. Consumir desde un único contexto de loop.

La calibración utiliza cinco puntos por cada uno de los cuatro presets. Se
invalida la sesión anterior al recalibrar, se guarda error por slot y se
continúa tras un fallo. Se mantienen GF inverso y fase circular. Compatibilidad
comprueba frecuencia, MCLK, excitación, PGA y settling, además de RFB y valores
numéricos válidos. Un slot inválido nunca se sustituye por otro automáticamente.

Un solo secuenciador crea un snapshot por ciclo: Pt100 0x41, standby y EC
0x81/0x82. No duplica la máquina DFT. Estados: Calibrating, Measuring, Stopping,
Idle y Error. Busy reserva también los periodos entre ciclos continuos. Stop
se completa por update; cal run durante continuo cancela y no reanuda.

Resultados conservan setup y metadatos completos de cada sensor. Un sensor
fallido no oculta al otro. La adquisición puede ser válida y la conversión
estar fuera de rango. Cancelación produce snapshot inválido explícito.
Un slot de resultados con `take` y contador de sobrescrituras evita duplicados
y pérdidas silenciosas; [contrato completo](../../docs/architecture.md).

Dependencias: ImpedanceApp, ImpedanceMath, Pt100, Conductivity y Config. Las
tres extensiones históricas del gestor y sus parsers/programadores separados
se retiraron. Las fórmulas de conversión siguen en sus módulos puros.
