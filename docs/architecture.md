# ATLANTES — arquitectura firmware y futura Web BLE

## Hito 3A: BLE GATT y WebApp

`main` conserva un único `CN0349Manager` compartido por Terminal y `BleApp`.
Después de `instrument.update()` se atienden ambas interfaces de forma
cooperativa. BLE solo consulta `getStatus()`/`isBusy()`; no consume el slot de
resultados, no inicia operaciones y no toca I²C. NimBLE gestiona GATT y GAP;
el firmware no crea tareas FreeRTOS propias ni introduce esperas.

```text
Terminal ─┐
          ├── CN0349Manager ── adquisición y conversiones existentes
BleApp  ──┘         │
                    └── STATUS READ + NOTIFY ── WebApp local
```

Nombre GAP: `CN0349`. Servicio ATLANTES:
`8d7b0001-6f7a-4f31-9c4d-2b6e6a7f3a10`. STATUS:
`8d7b0002-6f7a-4f31-9c4d-2b6e6a7f3a10`. Payload UTF-8 compacto:
`{"state":"READY","cal":4,"busy":false}`. `cal` es 0–4 y cuenta
calibraciones legacy válidas. El mapeo es NotInitialized→INIT,
Calibrating→CALIBRATING, Idle listo→READY, Measuring/Stopping→MEASURING y
Error/no readiness→ERROR. Se notifica al cambiar el payload y se reanuda
advertising tras desconexión.

La WebApp nativa lee y se suscribe a STATUS. `file://` puede no disponer de Web
Bluetooth según navegador; la UI lo detecta y recomienda localhost. Hito 3A no
define escrituras, Setup/Calibration/Measurement BLE, seguridad, bonding,
reconexión automática, persistencia, Wi-Fi ni servidor HTTP.

## Hito 2G: procesamiento EC y contrato BLE futuro

La adquisición DFT sigue siendo única. RAW usa impedancia legacy; SERIES resta
una vez R2; ANALOG_DEVICES usa NX. AD guarda `GF=(YH-YL)/(NH-NL)` y
`NOS=NH-YH/GF`, según ecuaciones 1 y 2 de la [CN-0349 Rev. A](https://www.analog.com/media/en/reference-design-documentation/reference-designs/cn0349.pdf).
LOW combina 0x22/0x12 y HIGH 0x11/0x09. La cadena `DFT→NX→YX→YCELL→EC` usa
ecuaciones 3 y 4 y cada rango falla por separado. La API añade estado/calibración
AD; el snapshot para BLE contiene todos los coeficientes, valores y errores.
BLE aún no se implementa.

Estado: Hito 2F, 2026-10-27, firmware 1.0.0. Implementación software comprobada
con dobles; validación física pendiente antes del Hito 3.

```text
Terminal / futura Web BLE
          |
CN0349Manager (controlador del instrumento)
          |
Setup / Calibration / Measurement
          |
ImpedanceApp -> MuxApp -> AD5934 / ADG715 (CN0349)
          |
       Pt100 / EC -> conversiones puras
```

Se simplifica `CN0349Manager`, que ya cubría el control, sin añadir otra clase
InstrumentController. No existen dependencias de Stream, Serial, Terminal ni
BLE en el controlador, adquisición o multiplexor. Una sola instancia de cada
driver y una sola máquina DFT atienden calibraciones y ambos sensores.

## Inicialización y calibración

`main` configura Serial y Wire (GPIO4/5, 100 kHz, timeout 25 ms), llama
`instrument.begin(busReady)` y después `terminal.begin()`. El arranque no espera
un monitor. `begin` comprueba ambos dispositivos, recupera/verifica standby y
programa la calibración. La preparación I²C es una secuencia finita acotada por
el timeout de Wire, sin esperas de DFT. `update` progresa cooperativamente.

Se adquieren cinco puntos por preset, por orden HIGH_100R, HIGH_1K, LOW_1K,
LOW_10K. Cada muestra recorre la misma máquina DFT y termina en standby.
Se conservan las fórmulas existentes: GF = 1/(RCAL · magnitud media), fase
instrumental por media circular, desviación estándar muestral y variación.
La frecuencia efectiva deriva de la palabra DDS y MCLK.

Un nuevo intento invalida los cuatro slots anteriores. Cada slot conserva
identificador de preset e intento, máscara, RFB, RCAL, configuración, cinco
muestras, GF, fase, tiempos, validez y errores. Un fallo invalida ese slot y se
continúa con el siguiente, recuperando hardware si es necesario; nunca se
selecciona otro preset automáticamente. Solo se utilizan slots con magnitud y
GF finitos y positivos y configuración compatible. Una calibración parcial
permite medir las selecciones que tengan sus slots válidos.

`runCalibration` comparte la lógica del arranque. Durante modo continuo solicita
primero cancelación segura, espera standby y comienza la calibración; permanece
detenido al terminar. Durante una medida puntual o calibración responde Busy.
Un fallo de recuperación invalida los slots anteriores y devuelve Hardware.

## Setup y límites

Estado inicial determinista y solo en RAM:

| Parámetro | Inicial | Modificación |
|---|---|---|
| Sensores | PT100 EC | `setMeasurementSensors` / `measure set` |
| Intervalo | 1000 ms | 1…2147483647 ms |
| Pt100 | 0x41, CAL_HIGH_100R | Fijo |
| EC | LOW 0x82, CAL_LOW_10K | HIGH 0x81 o LOW 0x82 |
| Modelo EC | RAW | Serie rechazada mientras no se verifique topología |
| K nominal | 1 cm⁻¹ | Finita y positiva |
| AD5934 | 2000 Hz, MCLK externo 1 MHz | Fijo en operación |
| Excitación / PGA / settling | Range 1, 2 Vpp nominal / ×1 / 10 ciclos ×1 | Fijo |

La API permite elegir cualquiera de los cuatro presets para EC si su RFB
coincide con la ruta. El comando de rango aplica de forma explícita el preset
HIGH_100R o LOW_10K. No hay autorange ni sustitución por un slot disponible.
Estos valores son operativos iniciales, no presets metrológicamente óptimos.

`setSetup` valida primero Busy y después el conjunto completo, sin aplicar
cambios parciales. La reserva Busy comprende también las esperas entre ciclos
continuos. No hay `setup sensor` alternativo.

Pt100 conserva IEC 60751 sin correcciones empíricas ni compensación de tres
hilos. EC conserva conductividad estimada K/Z aparente en RAW, sin restar R2,
sin compensación térmica ni soluciones patrón. La fórmula serie permanece en
la conversión pura para pruebas matemáticas, pero la API operativa la rechaza.
La calibración electrónica no valida la celda ni la exactitud de los sensores.

## Adquisición, secuenciación y tiempo

`ImpedanceApp` conserva configuración y readback por etapas, standby previo a
Initialize, settling por reloj, sondeo D1 (cada 5 ms), REAL, IMAG y parada.
El timeout DFT es 500 ms; configuración/control tienen límite de fase 1500 ms.
No hay `delay`, esperas activas, tareas FreeRTOS ni asignación dinámica.
Las operaciones I²C son síncronas pero acotadas; no se promete tiempo de loop
menor que el timeout del bus bajo fallo.

En cada ciclo se adquiere Pt100 si está seleccionado y, solo al terminar su
adquisición y verificar standby, se conmuta a EC. Las máscaras autorizadas y
el interlock de AD5934 se mantienen. Nunca hay dos rutas de sensor simultáneas.
Un fallo en Pt100 no impide intentar EC; se verifica recuperación antes de
conmutar. Un error EC conserva el resultado Pt100 del mismo ciclo.

Modo puntual: primer ciclo inmediato y vuelta a Idle al finalizar. Continuo:
primer ciclo inmediato; el siguiente comienza cuando el tiempo unsigned desde
el inicio real anterior alcanza el intervalo y no hay adquisición activa.
Se rebasa el origen en cada inicio real: si se excede el intervalo, se marca
`intervalExceeded`, se incrementa el contador y comienza un único ciclo en la
siguiente oportunidad de `update`. No hay cola ni recuperación de plazos
anteriores. El periodo efectivo es aproximadamente max(intervalo, duración del
ciclo), más la latencia del loop. Se usan diferencias unsigned de `millis`,
válidas al cruzar su desbordamiento; atender `update` regularmente.

`stopMeasurement` desactiva nuevos ciclos y solicita Stopping. La cancelación
se completa en siguientes `update`, verificando standby incluso si se detuvo
en power-down durante configuración. Se publica un snapshot cancelado e
inválido, conservando cualquier sensor ya completado. El terminal lo consume
sin mostrarlo como medida; anuncia `OK STOPPED` solo tras finalizar. Si falla
standby, se informa Hardware y no se declara hardware listo. Un error final de
hardware detiene el modo continuo; `cal run` permite recuperar y recalibrar.

## Resultados y consumo sin duplicaciones silenciosas

`MeasurementCycle` es un valor: ID incremental de sesión, inicio/fin en millis
de 32 bits, copia completa del setup, sensor Pt100 y sensor EC. Los tiempos y
IDs reinician al arrancar; los tiempos se interpretan módulo 2³². Cada sensor
conserva ruta, preset e ID de calibración, RFB/RCAL, GF, frecuencia, excitación,
PGA, settling, REAL/IMAG, magnitud, fases, Z, conversión, validez, error y tiempo.
La validez de adquisición es distinta de la de conversión: una Pt100 fuera del
rango IEC puede tener una adquisición DFT correcta. Un fallo no reutiliza el
valor anterior. El ciclo solo es válido si todos los sensores seleccionados
son válidos. Los snapshots publicados no se recalculan al modificar setup.

La API guarda el último ciclo completo, sin cola:

- `getMeasurementResult(out)` copia el último snapshot sin consumirlo.
- `takeMeasurementResult(out)` devuelve una vez el resultado pendiente y lo
  reconoce; una segunda llamada sin nueva publicación devuelve false.
- `publishedCycles`, `cycleId` y `overwrittenResults` permiten detectar pérdida:
  publicar sobre un resultado no consumido incrementa `overwrittenResults`.

El consumidor debe ejecutar `take` después de cada `update`, antes de la
siguiente publicación. Así no pierde ciclos aunque el intervalo sea mínimo:
una llamada de `update` nunca publica dos ciclos. El terminal actual es el
consumidor único. En Hito 3 un dispatcher único deberá consumir y distribuir
la misma copia a terminal y BLE; no deben competir ambos por `take`. Un cliente
BLE lento necesita detectar saltos por ID/contador y aplicar su política de
entrega explícita. No se garantiza almacenamiento histórico ilimitado.

## API para Hito 3

| Método CN0349Manager | Contrato |
|---|---|
| `begin(busReady)` | Inicializar y programar calibraciones |
| `update()` | Progreso cooperativo |
| `getStatus()` | Estado, readiness, calibraciones, contadores y error |
| `getSetup()` / `setSetup(setup)` | Copia / validación atómica |
| `runCalibration()` | Arranque o recalibración manual compartida |
| `getCalibrationStatus(preset, out)` | Snapshot incluso si es inválido |
| `setMeasurementSensors(selection)` | Pt100, EC o ambos |
| `startSingleMeasurement()` | Iniciar ciclo único |
| `startContinuousMeasurement()` | Iniciar ciclos periódicos |
| `stopMeasurement()` | Solicitar cancelación segura |
| `getMeasurementResult(out)` / `takeMeasurementResult(out)` | Consulta / consumo |

Se llama desde un único contexto cooperativo. No es una API thread-safe; los
futuros callbacks BLE deben entregar solicitudes al loop y formatear copias,
sin tocar directamente Wire, drivers ni rutas. No se implementan servicios
BLE, protocolo de transporte, persistencia NVS ni WebApp en este hito.

## Terminal y funcionalidades retiradas

Comandos finales sin distinción de mayúsculas:

```text
help
status
setup show
setup interval 1000
setup ec range high
setup ec range low
setup ec model raw
setup ec model series
setup ec k 1.0
cal status
cal show
cal run
measure show
measure set PT100
measure set EC
measure set PT100 EC
measure set EC PT100
measure once
measure start
measure stop
```

`series` devuelve `ERR INVALID_ARGUMENT` hasta verificar su topología.
Sensores desconocidos, repetidos o selección vacía se rechazan. Respuestas
tipadas: BUSY, INVALID_ARGUMENT, HARDWARE, NOT_CALIBRATED, TIMEOUT,
MEASUREMENT_FAILED; los snapshots cancelados usan Cancelled. Las medidas
muestran valores compactos; los errores por sensor impiden el sufijo OK.

Se retiran los comandos `cn0349 ...`, `ad5934 ...`, `adg715 ...`, escáner,
volcados, registro de módulos del terminal, barridos operativos, configuración
experimental de frecuencia/settling y el programador exclusivo de temperatura
continua. Sus estados/resultados duplicados y tres extensiones del gestor se
sustituyen por un ciclo y un secuenciador. Drivers y conversiones se conservan.
El historial en README y protocolos anteriores describe versiones anteriores;
la interfaz vigente es la de este documento.

Validación: [auditoría](hito2f-audit.md), [pruebas 2F](../test/hito2f.md).
