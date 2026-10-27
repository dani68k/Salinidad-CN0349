# CN0349Manager — API de aplicación (Hito 2A)

`CN0349Manager` es la puerta de alto nivel para terminal y futuras capas BLE.
Reutiliza una referencia a `ImpedanceApp`, que conserva el único motor DFT, la
coordinación AD5934/ADG715, los timeouts y la recuperación. El Manager no crea
drivers, no inicializa Wire, no accede a registros y no calcula magnitudes
metrológicas.

## Ciclo de vida

```text
Wire/Serial y drivers (main)
           |
      ImpedanceApp <--- CN0349Manager <--- Terminal / BLE futuro
           |                                  |
       AD5934 + ADG715                   API tipada
```

`begin(Terminal&, busReady)` registra el handler `cn0349` y almacena el estado
inicializado del bus; no sondea ni modifica hardware. `update()` delega al
`ImpedanceApp::update()` existente y sincroniza operation/outcome. `main` llama
una sola vez al Manager en el loop, por lo que no hay un segundo motor.

Los handlers heredados `ad5934` y `adg715` se conservan. La configuración se
consulta en vivo desde ImpedanceApp; `MuxApp` conserva la última máscara observada
por lectura/readback. Así el estado del Manager refleja cambios hechos por los
comandos anteriores. Consultas `getStatus()` y `getDiagnostics()` son snapshots
cacheados y no hacen I²C. `runDiagnostics()` está separado y sí solicita lecturas.

## API tipada

- `setPreset/getPreset`: preset solicitado para la siguiente operación; no lo
  aplica al hardware.
- `configureSingle(CN0349SingleConfig)` y `configureSweep(CN0349SweepConfig)`:
  validan busy, preset, límites de frecuencia, acumulación DDS y settling a través
  de las funciones/validadores existentes. Los defaults son 2 kHz y 1000 + 250 ×
  8 Hz, con CAL_LOW_1K y 10 settling cycles.
- `startSingle/startSweep`: solicitan al motor existente un trabajo asíncrono;
  el preflight I²C inicial es síncrono y acotado, sin esperar la conversión. La
  adquisición/polling sigue avanzando en `update()`.
- `cancel`: solicita la cancelación segura del motor; no simula terminación
  inmediata.
- `getStatus`: estado actual (`NOT_INITIALIZED`, `IDLE`, `RUNNING_SINGLE`,
  `RUNNING_SWEEP`, `ERROR`), operación y resultado histórico por separado,
  busy, readiness, presets solicitado/aplicado, máscara física observada, puntos,
  tiempo y errores.
- `getSingleResult/getSweepResult`: exponen DFT RAW con frecuencia solicitada y
  efectiva, REAL/IMAG, magnitud/fase, validez y tiempo. El puntero de puntos del
  barrido apunta a un snapshot fijo del Manager, sin heap; se reemplaza al pedir
  otro snapshot de barrido.
- `getDiagnostics`: snapshot del diagnóstico 1E, sin I²C. `runDiagnostics()` y
  `recover()` son acciones activas delegadas a ImpedanceApp.

Una adquisición completa deja estado actual IDLE y outcome histórico COMPLETE.
Los resultados se quedan en RAM y no son impedancia ni conductividad.

## Terminal

```text
cn0349 help
cn0349 status
cn0349 config
cn0349 preset list
cn0349 preset set CAL_LOW_1K
cn0349 measure single
cn0349 measure sweep
cn0349 measure stop
cn0349 result
cn0349 diag
cn0349 recover
```

La selección es software hasta que empieza la adquisición. `status/config` no
modifican preset. `diag/recover` hacen I²C mediante el subsistema existente.
Los nombres de preset usan la tabla única de MuxApp. Todos los comandos antiguos
siguen registrados.

## Evolución prevista

Los próximos subhitos podrán ampliar este contrato con calibración y unidades
ingenieriles sin cambiar el driver genérico. Aquí no se guardan coeficientes, no
se calcula ohmios/siemens/conductividad y no se introduce BLE ni web.

Pruebas e integración: [`../../test/hito2a.md`](../../test/hito2a.md).

## Hito 2B — Calibration Engine coordinado por CN0349Manager

El Manager añade una secuencia cooperativa de calibración con cinco adquisiciones de `ImpedanceApp` por preset. No contiene un motor DFT ni hace I2C. Guarda cuatro resultados independientes en RAM y sustituye un slot solo después de completar todas sus muestras válidas. `getCalibration(preset, out)` recupera el valor y señala compatibilidad con MCLK, frecuencia, excitación, PGA y settling actuales. No permite aplicar un coeficiente de otra ruta: los resultados están indexados por preset, que contiene la máscara y RFB/RCAL resueltos mediante `MuxApp`.

### Modelo y significado

Para magnitud `M = sqrt(REAL² + IMAG²)` y fase instrumental `atan2(IMAG, REAL)`, se calcula:

`GF = 1 / (RCALnominal × Mmedia)`

La hipótesis es una cadena lineal, estable y compatible con el modelo AD5934 de código DFT proporcional a la admitancia, con RCAL nominal conocida. El valor de RCAL no es una medición independiente. El GF conserva unidades inversas de ohm por código DFT (no ohmios); el código no realiza aún `MZ = 1/(GF×M)` para una celda. No se supone una respuesta idéntica entre rutas ni se compensa fase adicionalmente.

Cinco magnitudes se conservan individualmente. Se calculan promedio, desviación estándar muestral y desviación/media en porcentaje. Ningún punto se descarta por ser atípico y no hay límite arbitrario de aceptación. La fase media es circular (`atan2(sum sin, sum cos)`), evitando el promedio aritmético incorrecto cerca del cruce ±180°. Se guardan REAL/IMAG/fase/magnitud individuales y representativos, frecuencia solicitada/efectiva, MCLK, RFB, RCAL, excitación, PGA, settling, número de muestras, duración, ID y validez.

La compatibilidad actual compara MCLK, frecuencia solicitada, rango de excitación, PGA y settling. Un resultado incompatible sigue consultable para revisión, marcado `compatible=no`; no se debe usar su GF para convertir una medida. RFB/RCAL y la máscara distinguen rutas y se guardan con cada resultado.

### Comandos

```text
cn0349 cal help
cn0349 cal status
cn0349 cal run CAL_HIGH_100R
cn0349 cal run CAL_HIGH_1K
cn0349 cal run CAL_LOW_1K
cn0349 cal run CAL_LOW_10K
cn0349 cal run all
cn0349 cal result
cn0349 cal list
cn0349 cal stop
cn0349 cal clear
```

`run all` usa el mismo flujo single y secuencia los cuatro presets sin bloqueo de conversiones. `stop` solicita cancelación segura; la calibración anterior queda preservada. `clear` borra únicamente los cuatro slots de RAM y se rechaza durante actividad. Reiniciar pierde los coeficientes; no hay NVS.

Esto es calibración electrónica con resistencias internas nominales. No es calibración de celda: electrodos, cableado, solución, geometría, deriva y efectos electroquímicos requieren ensayos específicos del Hito 2C. La validación física y de repetibilidad sigue pendiente.

Procedimiento: [`../../test/hito2b.md`](../../test/hito2b.md).

## Hito 2C.1 — Medición aparente Pt100

`CN0349Impedance.cpp` implementa métodos de la misma clase CN0349Manager;
`ImpedanceMath.h` contiene cálculo puro y validación numérica. No hay otro motor,
Wire ni drivers. El estado de medida es IDLE/RUNNING/COMPLETE/FAILED/CANCELLED.
El estado general incorpora RUNNING_IMPEDANCE y la operación IMPEDANCE.

API pública añadida:

```cpp
CN0349ImpedanceError selectImpedanceCalibration(CN0349Preset);
CN0349Preset impedanceCalibration() const;
CN0349ImpedanceError startImpedance();
bool cancelImpedance();
bool isImpedanceRunning() const;
CN0349ImpedanceResult getImpedanceResult() const;
bool getLastValidImpedanceResult(CN0349ImpedanceResult&) const;
CN0349ImpedanceError impedanceRequestError() const;
```

`select` acepta manualmente las dos rutas HIGH incluso si aún no hay slot RAM;
`start` exige calibración disponible, compatible y perfil actual a 2000 Hz.
La selección no conmuta hardware. `cal list` informa disponibilidad y compatibilidad
para la medida, distinta de la compatibilidad genérica de `cn0349 cal list`.
Un rechazo previo no borra el resultado ni altera la operación activa; su motivo
queda en `impedanceRequestError()`. Un intento aceptado tiene su propio snapshot
(estado, error, DFT, magnitud, fase, frecuencia solicitada/efectiva, configuración,
ID/máscara/fase/GF de calibración y duración). El getter devuelve el intento actual;
el segundo getter conserva el último válido incluso tras fallo/cancelación.
La salida de terminal distingue ambos e imprime ohmios solo con `valid=true`.
Los valores DFT de un intento que no llegó a completar la adquisición no son datos válidos.

Se copia la calibración al inicio y jamás se escribe en los cuatro slots durante
medición. Busy cubre también la finalización y rechaza clear, calibración u otras
adquisiciones. Las consultas son snapshots sin I²C; start tiene el preflight
síncrono acotado del motor existente y la espera DFT progresa en `update()`.
COMPLETE solo se publica después del standby verificado por ImpedanceApp.
Un fallo de standby produce FAILED; no se puede garantizar el estado físico
cuando falla el bus. Cancelar espera el final seguro; un fallo al detener prevalece
sobre CANCELLED. Se admite stop propio, cancel general y stop heredado AD5934.

Comandos:

```text
cn0349 impedance help
cn0349 impedance config
cn0349 impedance cal list
cn0349 impedance cal set CAL_HIGH_100R
cn0349 impedance cal set CAL_HIGH_1K
cn0349 impedance start
cn0349 impedance status
cn0349 impedance result
cn0349 impedance stop
```

[Modelo, unidades y circuito equivalente](../../docs/pt100-route.md).
[Pruebas físicas](../../test/hito2c1.md).

## Hito 2D — temperatura Pt100, 2026-10-27

`CN0349Temperature.cpp` añade `startTemperature`, `cancelTemperature`,
`getTemperatureResult` y `temperatureRequestError`, además del parser
`cn0349 temperature help|start|status|result|stop`. Reutiliza `startImpedance`
sin seleccionar ni recalibrar automáticamente; la selección sigue siendo
`cn0349 impedance cal set CAL_HIGH_100R|CAL_HIGH_1K`.

`update` avanza el motor DFT existente y, al finalizar, copia su resultado al
`CN0349TemperatureResult`. La función `convertPt100Acquisition` solo convierte
adquisiciones COMPLETE, valid, sin error de adquisición ni hardware. La estructura
anida todos los datos originales (ID/preset/máscaras, GF, fases, REAL/IMAG/magnitud,
Z, configuración, frecuencia efectiva y tiempo), más `Pt100::Result` y validez
global. Una adquisición correcta fuera del rango IEC conserva su estado COMPLETE
y Z válida, pero temperatura inválida con error matemático OUT_OF_RANGE.

Los errores hardware están en `acquisition.hardwareError`; hardware no listo,
calibración ausente/incompatible, adquisición y cancelación se distinguen en
`acquisition.error`; los matemáticos en `conversion.error`. Una conversión
no ejecutada devuelve NOT_ATTEMPTED y NaN. No se imprime temperatura inválida.
Los rechazos BUSY conservan el intento en curso y se informan por separado como
último error de solicitud. Los rechazos previos al inicio muestran ID=0.

El snapshot se devuelve por valor y solo una nueva solicitud de temperatura
aceptada para procesamiento lo reemplaza. Cambios de selección, recalibraciones
y adquisiciones de impedancia posteriores no lo reinterpretan. `stop` solo
cancela adquisiciones iniciadas por temperatura; la recuperación/standby conserva
el flujo asíncrono original. El estado de Manager durante la medida sigue siendo
RUNNING_IMPEDANCE, y el estado específico se consulta por la API de temperatura.

No hay corrección de cables; `cableCompensationApplied=false` reserva la
política futura sin modificar Z original. La conversión pura y sus límites se
describen en [temperature](../temperature/README.md); la cadena completa y el
ensayo con multímetro, en [hito2d](../../test/hito2d.md).

## Preparación automática — 2026-10-27

`prepareMeasurements()` se invoca por defecto desde setup después de iniciar
drivers y bus. Rechaza estado ocupado, ejecuta `recover()` y, si tiene éxito,
`startCalibrationAll(2000)`. Retornar true significa secuencia iniciada, no
calibración terminada. La recuperación conserva su implementación síncrona
acotada por los timeouts I2C existentes; las veinte adquisiciones siguen siendo
asíncronas. No se añaden esperas en loop ni reintentos automáticos.

`startCalibrationAll(frequencyHz)` centraliza el mismo flujo usado por el
comando manual `cn0349 cal run all`: HIGH_100R, HIGH_1K, LOW_1K, LOW_10K,
cinco muestras cada uno. Mantiene la selección de calibración RTD. Durante
calibración rige el busy existente; `cal stop` puede cancelarla. `update` observa
la finalización y solo anuncia listo con calibración COMPLETE, hardware listo
y slot seleccionado compatible. Un fallo de recovery aborta antes de calibrar;
fallo/cancelación posterior no anuncia listo. Los slots ya completados pueden
seguir presentes en una secuencia parcial, según el comportamiento previo.

Esta política de arranque autoriza nuevas calibraciones en RAM cada reinicio;
no cambia la política de `temperature start`, ni aplica ajustes empíricos o
compensaciones. No requiere sensor externo para calibrar las rutas internas.

## Temperatura continua — 2026-10-27

`cn0349 temperature continuous` / `startContinuousTemperature()` inicia una
primera adquisición inmediata y reserva el subsistema hasta `temperature stop`
o `cancelTemperature()`. `isContinuousTemperature()` y `temperature status`
permiten consultar el modo. El comando `start` conserva su medida única.

`scheduleTemperature()` usa resta unsigned de `millis()` con intervalo 1000 ms
entre inicios, tolerando el desbordamiento del contador. Solo inicia otra medida
cuando la anterior ha finalizado; si el loop o una adquisición tarda más de un
segundo, se retrasa el siguiente inicio sin acumular solicitudes ni solapar DFT.
El intervalo es cooperativo, no una garantía de temporización de tiempo real.
No se añaden delay ni bucles de espera.

El busy incluye la reserva durante los huecos entre muestras: no se permite
escanear I2C, recalibrar ni cambiar selección hasta detener el modo. Internamente
el planificador libera la reserva solo durante la llamada síncrona al mismo
`startTemperature` existente; no hay tareas concurrentes dentro de esa llamada.
Cada adquisición sigue tomando su propio snapshot de calibración.

Al completar cada muestra se imprime T, Z, preset/ID, REAL/IMAG/magnitud,
frecuencia efectiva, fase instrumental, duración y errores. La cabecera conserva
el aviso experimental; `temperature result` muestra el detalle completo del
último intento. Un resultado inválido o rechazo detiene el modo e informa el
error, sin recovery ni recalibración automática. `stop` durante una adquisición
solicita standby; entre adquisiciones conserva el último resultado. El stop
genérico del Manager también detiene el modo continuo. Estado inicial: desactivado.

### Salida continua simplificada — 2026-10-27

Cada muestra válida imprime exclusivamente `temperature: xx.xxºC`, con dos
decimales. `startTemperature(quiet)` y `startImpedance(quiet)` propagan una opción
por adquisición que el modo continuo activa tanto en la primera muestra como en
las siguientes. Suprime mensajes informativos de selección RTD, inicio DFT,
resultado RAW, standby correcto y resumen de impedancia. La opción por defecto
es false: las medidas únicas y comandos anteriores conservan sus mensajes.
Los errores y la parada siguen siendo visibles; la cabecera de activación
mantiene el aviso experimental. Todos los datos siguen en el snapshot y pueden
consultarse mediante `cn0349 temperature result`.

## Hito 2E.1 — EC con resistencias externas, 2026-10-27

`CN0349Ec.cpp` coordina `cn0349 ec help|config|range list|range set HIGH|LOW|
cal list|cal set <PRESET>|start|status|result|stop` y la API pública
`selectEcRange`, `selectEcCalibration`, `startEc`, `cancelEc`,
`getEcResult`, `getLastValidEcResult` y `ecRequestError`. HIGH usa `0x81`
con RFB 100 ohm; LOW usa `0x82` con RFB 1000 ohm. El cambio de ruta conserva
la calibración seleccionada; `config` señala incompatibilidad y `start` la
rechaza hasta una selección explícita. RTD, temperatura y EC mantienen estados
y resultados propios. El terminal solo solicita/consulta; no duplica DFT.

El arranque EC captura un slot 2B completo y comprueba máscara/RFB de la
calibración, GF, MCLK, DDS efectiva, rango, PGA, settling y reloj externo a
2000 Hz. `ImpedanceApp::requestEc` selecciona la ruta con readback ADG715 y
reutiliza su única máquina DFT/standby. Al terminar, el Manager verifica ruta
y RFB informados por `ImpedanceApp`, calcula `1/(GF*M)` y fase instrumental
normalizada. El snapshot `CN0349EcResult` contiene ruta, RFB, todos los campos
de adquisición/calibración, estado final AD5934 y bandera de standby. Una
selección posterior no recalcula el resultado. Los errores de solicitud y de
adquisición se separan; un intento fallido conserva el último válido aparte.
El resultado se presenta como `EC PROBE - EXPERIMENTAL / NOT ABSOLUTELY
CALIBRATED`. No se sustrae R2, no se calcula conductividad ni se afirma ausencia
de saturación (`saturationKnown=false`). La lectura digital de máscara no
valida la continuidad analógica. [Topología e incertidumbre](../../docs/ec-route.md),
[matriz de 40 medidas](../../test/hito2e1.md).

## Hitos 2E.2 y 2E.3 — Sonda EC real y conductividad estimada

El mismo Manager y la misma máquina DFT admiten la sonda conectada a J1 sin una
API nueva. `startEc` congela ruta, máscara, RFB, preset, ID, GF y parámetros de
adquisición; `advanceEc` publica un resultado válido solo tras DFT y standby
verificados. `getEcResult` expone el último intento y `getLastValidEcResult`
conserva el último válido por separado. La terminal permite seleccionar ruta y
calibración manualmente; un cambio de ruta no cambia el preset en secreto.

El encabezado de `ec result` identifica la medida como experimental de sonda.
`cn0349 conductivity start` reutiliza `startEc` y conserva un snapshot EC junto
al resultado de `Conductivity::convert`, con modelo RAW, K y R2 nominales.
`conductivity result` muestra DFT, Z aparente y la estimación en µS/cm y mS/cm;
si falla la adquisición o la conversión, no presenta cifras como válidas. El
modo `SERIES_R2_EXPERIMENTAL` aparece en la ayuda pero se rechaza hasta verificar
la topología real de R2 en la placa. Un rechazo previo al inicio conserva el
último resultado. El snapshot queda en RAM y es la frontera de datos para una
futura exportación BLE; no almacena identidad de muestra ni temperatura medida
con multímetro. Esos metadatos se anotan en el [protocolo](../../test/hito2e2.md).
No hay corrección de R2, autorange ni detección fiable de saturación. Los 2 Vpp
de Range 1 son nominales en el AD5934 y no certifican la tensión en electrodos.
