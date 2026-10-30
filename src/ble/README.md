# BLE GATT

BleApp comparte CN0349Manager con Terminal. `begin()` crea el servicio y el
advertising; `update()` procesa solicitudes y publica snapshots. Los callbacks
encolan órdenes sin acceder al bus I²C. Dependencias: CN0349Manager y
NimBLE-Arduino 2.x. El nombre anunciado es `CN0349` y el servicio ATLANTES es
`8d7b0001-6f7a-4f31-9c4d-2b6e6a7f3a10`.

## Control y resultados

- MEASUREMENT `8d7b0007-6f7a-4f31-9c4d-2b6e6a7f3a10`: READ + NOTIFY, 26 bytes.
- CONTROL_COMMAND `8d7b0008-6f7a-4f31-9c4d-2b6e6a7f3a10`: WRITE, 4 bytes.
- CONTROL_RESULT `8d7b0009-6f7a-4f31-9c4d-2b6e6a7f3a10`: READ + NOTIFY, 4 bytes.

CONTROL_COMMAND es `version, requestId, operation, argument`. Operaciones:
1 CAL_RUN, 2 SET_SENSORS (argumento 1=PT100, 2=EC, 3=BOTH), 3 ONCE, 4 START y
5 STOP. CONTROL_RESULT devuelve `version, requestId, operation, result`; los
códigos distinguen longitud/versión/operación, NOT_READY, BUSY, argumentos,
hardware, falta de calibración, timeout, fallo de medida y cancelación. Solo se
encola un comando; su ejecución ocurre en `update()` mediante las APIs públicas
del Manager. CAL_RUN, selección, ONCE y START requieren READY; STOP se admite
para una medida o continuo activos.

MEASUREMENT es little-endian: versión (0), selección (1), cycleId uint32 (2),
estado/error del ciclo (6–7), estado/error Pt100 (8–9), temperatura float32
(10), estado/error EC (14–15), modelo (16), error Conductivity (17), µS/cm float32 (18)
y mS/cm float32 (22). Estado de sensor: 0 no seleccionado, 1 válido, 2 error.
La temperatura interna Pt100 es double y se convierte explícitamente a float32
al serializar; no se copian bytes parciales de la representación double.
BLE consulta el snapshot mediante `getMeasurementResult()` y
`publishedCycles`; no consume ni compite con el slot usado por Terminal.

## Lectura de calibraciones

CALIBRATION `8d7b0006-6f7a-4f31-9c4d-2b6e6a7f3a10` es READ y contiene un
registro binario fijo de 76 bytes, little-endian. Las lecturas largas GATT se
delegan al procedimiento estándar de NimBLE; no hay JSON ni fragmentación de
aplicación.

| Offset | Tamaño | Campo |
|---:|---:|---|
| 0 | 1 | versión (=1) |
| 1 | 1 | estado general: 0 ABSENT, 1 CALIBRATING, 2 VALID, 3 PARTIAL, 4 INVALID |
| 2 | 4 | calibrationRunId, uint32 |
| 6 | 36 | cuatro registros en orden HIGH_100R, HIGH_1K, LOW_1K, LOW_10K |
| 42 | 17 | AD HIGH |
| 59 | 17 | AD LOW |

Cada referencia ocupa 9 bytes: `state:u8`, `meanMagnitude:f32` y
`legacyGF:f32`. `state` conserva CalibrationState: 0 INVALID, 1 RUNNING,
2 VALID. Cada rango AD ocupa 17 bytes: `valid:u8`, `GF:f64` y `NOS:f64`.
Float32/float64 son IEEE-754. Los valores se copian de los snapshots existentes;
BLE no recalcula coeficientes.

Durante CALIBRATING se publica el encabezado con registros a cero para impedir
lecturas parciales. Tras incrementar `calibrationRunId`, `update()` captura la
sesión terminada y actualiza la característica. ABSENT/INVALID informan
explícitamente que no existe una calibración utilizable. La característica no
notifica y no inicia calibraciones.

## Protocolo Setup

Características:

- SETUP `8d7b0003-6f7a-4f31-9c4d-2b6e6a7f3a10`: READ + NOTIFY, 12 bytes.
- SETUP_COMMAND `8d7b0004-6f7a-4f31-9c4d-2b6e6a7f3a10`: WRITE, 13 bytes.
- SETUP_RESULT `8d7b0005-6f7a-4f31-9c4d-2b6e6a7f3a10`: READ + NOTIFY, 3 bytes.

Todos los enteros y float32 IEEE-754 usan little-endian. SETUP contiene:
`version:u8, sensors:u8, interval:u32, range:u8, model:u8, K:f32`.
SETUP_COMMAND inserta `requestId:u8` después de version. Valores: sensors
1=PT100, 2=EC, 3=ambos; range 0=LOW, 1=HIGH; model 0=RAW, 1=SERIES, 2=AD.
El tamaño fijo cabe en el payload ATT mínimo y evita fragmentar JSON.

SETUP_RESULT es `version, requestId, result`; códigos: 0 OK, 1 INVALID_LENGTH,
2 UNSUPPORTED_VERSION, 3 INVALID_VALUE, 4 NOT_READY, 5 BUSY y 6 INTERNAL_ERROR.
El callback solo copia una petición fija; `update()` exige Idle/readiness,
decodifica y llama una vez a `setSetup()`. Así no hay aplicación parcial ni
acceso concurrente al Manager. SETUP se notifica tras APPLY y cuando Terminal
cambia la configuración. No se aceptan órdenes fuera de READY. Solo se admite
una petición pendiente; una escritura adicional recibe BUSY.

`BleApp` expone en modo periférico el estado de `CN0349Manager`; no contiene
lógica de calibración o medida. `begin()` crea GATT y advertising, y `update()`
compara un JSON compacto con el anterior. Solo cambia la característica y
notifica cuando cambia algún campo. No usa `delay`, tareas propias ni Serial.

- Nombre anunciado: `CN0349`.
- Servicio ATLANTES: `8d7b0001-6f7a-4f31-9c4d-2b6e6a7f3a10`.
- Característica STATUS: `8d7b0002-6f7a-4f31-9c4d-2b6e6a7f3a10`.
- Propiedades STATUS: READ + NOTIFY.

Formato UTF-8: `{"state":"READY","cal":4,"busy":false}`. `cal` es el número
real de calibraciones legacy válidas (0–4); `busy` procede de `isBusy()`.
NotInitialized se publica como INIT, Calibrating como CALIBRATING, Idle listo
como READY, Measuring/Stopping como MEASURING y cualquier error/no readiness
como ERROR. NimBLE reanuda advertising al desconectar.

No se implementan bonding ni reconexión automática del cliente.

La interfaz pública de `BleApp` y los identificadores, tamaños y códigos de
`AtlantesBle` forman parte de la referencia HTML generada con `doxygen Doxyfile`.

Al desconectarse el último central, el callback descarta las órdenes de control
BLE pendientes y solicita una parada. `update()` ejecuta `stopMeasurement()`
si hay una medición activa o continua; la parada sigue el flujo habitual del
controlador. Las mediciones iniciadas posteriormente desde Terminal pueden
funcionar sin un central conectado.

## Advanced Setup

ADVANCED `8d7b000a-6f7a-4f31-9c4d-2b6e6a7f3a10` ofrece READ + NOTIFY.
Su paquete de 20 bytes little-endian contiene version u8, MCLK, frecuencia inicial
y paso (tres u32 en Hz), numero de incrementos y settling (dos u16), rango y
PGA (u8), y Vpp nominal en decimas (u8; 0 si no esta disponible). Son valores
del perfil configurado, no lectura de registros. `update()` publica cambios sin
acceso I2C. El modo se deduce del numero de incrementos.

## Diagnóstico de rango EC

EC_DIAGNOSTIC `8d7b000b-6f7a-4f31-9c4d-2b6e6a7f3a10` ofrece READ + NOTIFY
y conserva intacto MEASUREMENT. Sus 7 bytes little-endian son `version:u8`,
`cycleId:u32`, `diagnostic:u8` y `range:u8`. Diagnóstico: 0 InvalidCal,
1 InRange, 2 NearLimit, 3 BelowRange y 4 AboveRange. Rango: 0 LOW, 1 HIGH,
255 EC no seleccionado. El diagnóstico se publica con el mismo ciclo que la
medición; una medida EC inválida se transmite como InvalidCal.

## Compensación de temperatura

SETUP_EXT `8d7b000c-6f7a-4f31-9c4d-2b6e6a7f3a10` es READ + NOTIFY y
contiene los 12 bytes de SETUP seguidos de `enabled:u8` y `alpha:f32`.
SETUP_EXT_COMMAND `8d7b000e-6f7a-4f31-9c4d-2b6e6a7f3a10` es WRITE y
contiene los 13 bytes de SETUP_COMMAND seguidos de esos mismos campos. Utiliza
SETUP_RESULT y el mismo proceso de validación y aplicación del controlador.

EC25 `8d7b000d-6f7a-4f31-9c4d-2b6e6a7f3a10` es READ + NOTIFY: `version:u8`,
`cycleId:u32`, `enabled:u8`, `valid:u8`, `ec25MicroSiemens:f32`, en
little-endian. El valor solo es utilizable si `valid` es 1. SETUP y MEASUREMENT
anteriores conservan sus formatos y UUID.
