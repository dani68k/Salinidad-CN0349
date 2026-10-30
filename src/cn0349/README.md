# CN0349Manager

Controlador de Setup, calibración y ciclos Pt100/EC. Comparte un único
`ImpedanceApp` y no depende de Serial ni de BLE. Sus operaciones se atienden
desde el contexto cooperativo de `update()`.

## Interfaz

| Función | Propósito y resultado |
|---|---|
| `begin(busReady)` | Carga Setup, comprueba hardware y arranca calibración; devuelve `InstrumentError`. |
| `update()` | Avanza calibración, adquisición y ciclos. |
| `getStatus()`, `isBusy()` | Copia de estado/readiness, contadores y ocupación. |
| `getSetup()`, `getImpedanceProfile()` | Copias de Setup y perfil configurado. |
| `setSetup(setup)`, `setMeasurementSensors(sensors)` | Validan y aplican la configuración; devuelven `InstrumentError`. |
| `runCalibration()` | Solicita calibración; si hay continuo, primero solicita parada. |
| `getCalibrationStatus(preset, out)`, `getAdCalibrationStatus(range, out)` | Copian una referencia o calibración AD; `false` indica argumento inválido. |
| `startSingleMeasurement()`, `startContinuousMeasurement()`, `stopMeasurement()` | Inician o detienen medidas; devuelven `InstrumentError`. |
| `getMeasurementResult(out)` | Copia el último ciclo sin consumirlo, si existe. |
| `takeMeasurementResult(out)` | Consume una vez el resultado pendiente; `false` si no existe. |

## Setup y persistencia

`InstrumentSetup` incluye sensores, intervalo, rango, preset de calibración EC,
modelo, K y compensación de temperatura. Valores iniciales: ambos sensores,
1000 ms, LOW, CAL_LOW_10K, RAW, K=1 cm⁻¹, compensación OFF y alpha=2 %/°C.

`setSetup()` rechaza cambios mientras hay trabajo activo; valida selección,
intervalo 1–2147483647 ms, rango/preset compatibles, modelo, K finita positiva
y alpha 0–5 %/°C con dos decimales. Con compensación activa exige ambos
sensores. Guarda los cambios en Preferences, namespace `cn0349`, blob versión
3; `loadSetup()` admite formatos anteriores. Si guardar falla, Setup permanece
aplicado en RAM y se devuelve `Persistence`. No se escribe si no cambian los
campos persistidos; las calibraciones y mediciones no se persisten.

## Calibración y ciclos

Se recogen cinco muestras por cada referencia HIGH_100R, HIGH_1K, LOW_1K y
LOW_10K. Se generan GF legacy y, cuando los dos puntos de cada rango son
válidos, GF/NOS del modelo AD. La compatibilidad comprueba la configuración
y los valores relevantes. Una referencia fallida no se reemplaza por otra.

Cada `MeasurementCycle` conserva un ID, Setup congelado y resultados por
sensor. Pt100 se adquiere antes de EC cuando ambos están seleccionados; un
fallo de uno no borra el resultado del otro. Un ciclo cancelado se marca como
tal. `publishedCycles` y `overwrittenResults` permiten observar publicaciones
y resultados no consumidos; BLE usa la copia no consumidora.

EC25 se calcula al finalizar el ciclo si la compensación está activa y ambas
lecturas son válidas: `EC25 = EC / (1 + alpha / 100 * (T - 25))`.
`ec25Valid` indica si hay un resultado finito con denominador positivo.

`ecRangeDiagnostic` compara la magnitud DFT con las dos referencias válidas y
compatibles del rango activo. Fuera del intervalo produce `BelowRange` o
`AboveRange`; el último 15 % hacia el máximo es `NearLimit`, el resto es
`InRange` y una calibración o lectura no válida produce `InvalidCal`. No
detecta saturación física ni cambia el rango.

El perfil AD5934 expuesto es configuración, no una lectura actual de hardware.
Ver [README principal](../../README.md) y [validación](../../test/VALIDACION.md).

## Referencia Doxygen

La API y los tipos públicos de `CN0349Manager.h` están documentados con
comentarios Doxygen. Desde la raíz, crear `build/doxygen` si aún no existe y
ejecutar `doxygen Doxyfile`; la referencia HTML se genera en
`build/doxygen/html/index.html`. La configuración incluye CN0349Manager,
ImpedanceApp, MuxApp, Pt100, Conductivity, AD5934, ADG715, BleApp y Terminal,
las cabeceras compartidas de configuración, matemática y diagnóstico, y la
página inicial.

La generación incorpora diagramas SVG de colaboración y de dependencias entre
cabeceras mediante Graphviz. Se extraen también miembros privados para mostrar
relaciones internas. `DOT_PATH` en `Doxyfile` apunta a la instalación local de
Graphviz; debe ajustarse en equipos con otra ubicación.
