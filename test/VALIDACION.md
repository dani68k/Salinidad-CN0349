# ATLANTES CN0349 — Evidencias de validación

Este documento reúne resultados y limitaciones registrados durante el desarrollo.
Una compilación confirma integración del código, una prueba con dobles comprueba
el comportamiento simulado y un ensayo físico comprueba únicamente el montaje,
la versión y las condiciones indicados. Ninguna de ellas demuestra por sí sola
exactitud metrológica. Los identificadores, comandos y versiones de los
registros históricos pueden no corresponder al firmware actual.

## Evidencia de software histórica

| Etapa | Registro disponible | Alcance |
|---|---|---|
| 1A–1E | Compilaciones ESP32-S3 y pruebas matemáticas preparadas; parte de las pruebas nativas no se ejecutaron por falta de GCC/G++ en aquel entorno | Drivers, DDS, control y secuencias de versiones históricas |
| 2D | Pruebas Pt100 3/3 y AD5934 11/11 ejecutadas con MSVC/Unity; cuatro grupos host reportados | Matemática pura y dobles; no valida sensores |
| 2F | Siete grupos host PASS, Pt100 3/3 y AD5934 11/11; compilación ESP32-S3 SUCCESS | Manager, secuencias, errores, terminal y regresiones con Wire simulado |
| 2G | Pruebas host y compilación ESP32-S3 reportadas; suite host ampliada con un octavo grupo | Modelo AD y regresiones en simulación |

Se retiraron las suites host y Unity durante la consolidación documental. Las
cifras anteriores son resultados históricos, no una ejecución de la versión
actual. Los dobles no reproducen la cadena analógica, el sensor, el reloj físico,
la latencia real ni la precisión metrológica.

## Pruebas físicas históricas

### I²C y adquisición

- Hito 1A: se reportaron ACK en 0x0D y 0x48 y funcionamiento de los comandos
  generales del terminal. El ACK no identifica el componente ni valida el reloj.
- Hito 1C: se reportó readback del ADG715 0x12 y 0x81. El readback confirma
  el registro, no la continuidad eléctrica de los contactos.
- Hito 1D.1: con firmware 0.4.1 se reportó una adquisición válida a 2 kHz,
  REAL −3405, IMAG −7513, magnitud 8248,587 y standby final. El intento
  anterior 0.4.0 había terminado por timeout DFT; no produjo una medida válida.
- Hitos 2B y posteriores: se reportaron calibraciones electrónicas y medidas
  con las rutas correspondientes. Su validez depende de la configuración y del
  montaje de cada sesión.

### Ruta Pt100 — Hito 2C.1

Se registraron ensayos con cargas nominales de 220 Ω y 2,2 kΩ, con referencias
de multímetro reportadas de 215 Ω y 2150 Ω. Se compararon calibraciones
HIGH_100R y HIGH_1K. La matriz inicial incluyó cuatro muestras preliminares;
posteriormente se registró una serie de cinco adquisiciones por combinación.
Las muestras preliminares y la serie de repetibilidad no deben mezclarse.

Ejemplos preliminares: para 215 Ω, HIGH_1K produjo 220,620987 Ω; para 2150 Ω,
HIGH_100R produjo 2022,875488 Ω y HIGH_1K 2121,152832 Ω. La diferencia entre
calibraciones mostró que la repetibilidad no equivale a exactitud absoluta.
Se conserva la transcripción y sus condiciones en el anexo.

El plan 2D recoge validación matemática de IEC 60751, pero dejaba pendiente una
comparación física de temperatura con referencia independiente. Los ensayos de
resistencias y la lectura de una Pt100 no demuestran una precisión de
temperatura ni compensación de cables.

### Ruta EC — Hito 2E.1

Se documentaron **40 adquisiciones** A–H: cinco por combinación de carga,
ruta y referencia. Se usaron 220 Ω y 2,2 kΩ nominales, sin medida actual
de ambas resistencias con multímetro. Perfil reportado: MCLK 1 MHz, 2 kHz,
Range 1, PGA ×1 y settling 10. Las filas se registraron como COMPLETE/VALID,
sin error de hardware y con standby verificado.

| Serie | Carga nominal | Ruta / calibración | Media Z aparente (Ω) | Desviación muestral (Ω) |
|---|---:|---|---:|---:|
| A | 220 Ω | HIGH / HIGH_100R | 306,459 | 0,059 |
| B | 220 Ω | HIGH / HIGH_1K | 321,285 | 0,049 |
| C | 220 Ω | LOW / LOW_1K | 435,105 | 0,015 |
| D | 220 Ω | LOW / LOW_10K | 442,578 | 0,026 |
| E | 2,2 kΩ | HIGH / HIGH_100R | 2112,163 | 1,447 |
| F | 2,2 kΩ | HIGH / HIGH_1K | 2214,809 | 3,423 |
| G | 2,2 kΩ | LOW / LOW_1K | 2248,125 | 0,138 |
| H | 2,2 kΩ | LOW / LOW_10K | 2287,046 | 0,311 |

La dispersión relativa máxima reportada fue 0,155 % en F. Las diferencias
entre rutas y calibraciones fueron mayores que la dispersión dentro de cada
serie. Comparar con carga nominal + 100 Ω es una hipótesis orientativa, no
una corrección verificada ni una demostración de exactitud. Se conservan las
40 filas y las condiciones en el anexo.

### Sonda EC y modelos

El registro 2E.2/2E.3 contiene sesiones exploratorias con agua del grifo y agua
con sal, junto con tablas y condiciones incompletas. Esas muestras no son
patrones certificados. El documento combina resultados parciales con apartados
de ensayos aún pendientes; no debe interpretarse como validación metrológica.
Se conserva el registro en el anexo.

El plan 2G para el modelo AD enumeraba pruebas físicas con 220 Ω, 2,2 kΩ, agua
y agua con sal como pendientes en el momento de redactarlo. Las pruebas de un
hito anterior no prueban automáticamente una versión posterior.

## BLE, WebApp y compensación

El desarrollo posterior incorporó BLE, Setup, calibración, medida, gráfica,
persistencia, diagnóstico de rango y compensación EC25. Se comunicó el cierre
funcional de los hitos 3H y 3I, y la revisión de las curvas EC y EC25.
No se dispone aquí de una matriz detallada de resultados o de referencias
metrológicas para esas validaciones. El cierre funcional no acredita
incertidumbre, exactitud ni calibración de la sonda.

## Limitaciones y pruebas pendientes de documentar

- Contrastar esquema de fabricación, conexiones, rutas analógicas y efecto de
  R2 en la placa concreta; los esquemas disponibles son simplificados.
- Caracterizar Pt100 y cables con una referencia térmica independiente.
- Medir soluciones patrón trazables y caracterizar K, repetibilidad, estabilidad,
  exactitud y compensación térmica para la sonda y el líquido usados.
- No interpretar el diagnóstico de rango DFT como detección física de
  saturación del ADC.
- Al documentar una nueva prueba, anotar firmware/commit, placa, fecha,
  conexiones, patrones, temperatura, configuración, resultado y criterio.

## Anexos: registros originales de banco

Los siguientes anexos conservan el contenido de tres registros históricos,
incluidos resultados crudos, planes y observaciones de su momento. Sus
referencias cruzadas y comandos describen aquella versión y pueden estar
obsoletos. Las secciones de planes no son evidencia de ejecución.

### Registro original: hito2c.1_testing.md

````text
# Hito 2C.1 — Prueba paso a paso desde el arranque

**Objetivo:** hacer cinco medidas con cada calibración para cada resistencia.
Primero probaremos la de **2k2** y después la de **220 Ω**: 20 medidas en total.

**Estado: CERRADO — 2026-10-27.** Se recibieron las veinte medidas y los diagnósticos finales. El procedimiento y los resultados quedan aquí como registro; las instrucciones describen cómo se ejecutó la prueba.

Ejecuta los comandos **uno a uno**, pulsando Intro después de cada comando.
Las indicaciones «qué pegar» se refieren a los espacios del apartado 7 de este
mismo documento. También puedes pasar las salidas por el chat para registrarlas aquí.

## 1. Conectar la resistencia y arrancar

1. Con la placa apagada, mide la resistencia con el multímetro y anota su valor.
2. Conecta solo la resistencia de **2k2** entre PT100-1 y PT100-2 de J1,
   previamente identificados en la placa. No uses los terminales de conductividad.
3. Enciende la placa y abre el monitor serie a **115200 baudios**.
4. Espera a que aparezca `Sistema preparado`.

**Qué pegar:** el valor del multímetro y toda la salida de arranque,
desde `ATLANTES CN-0349` hasta `Sistema preparado`.

## 2. Preparar y comprobar el sistema

Ejecuta:

```text
cn0349 recover
```

Espera `Manager recovery: COMPLETE`. Después ejecuta:

```text
cn0349 diag
```

Debe aparecer `Manager diagnostic: READY`. Después ejecuta:

```text
cn0349 impedance config
```

Comprueba: ruta `0x41`, frecuencia 2000 Hz, MCLK 1000000 Hz, Range 1,
PGA 1 y settling 10. Si no coincide o aparece un error, pega la salida
para revisarla antes de seguir.

**Qué pegar:** los tres comandos y sus respuestas completas.

## 3. Calcular los GF (una vez para esta resistencia)

Ejecuta:

```text
cn0349 cal run all
```

Espera a que terminen las adquisiciones de los cuatro presets. No inicies
una medida externa todavía. Ejecuta:

```text
cn0349 cal status
```

Si dice RUNNING, espera y consulta otra vez. Continúa solo cuando diga
`Calibration state: COMPLETE` y el error de calibración sea `NONE`.
Después ejecuta:

```text
cn0349 impedance cal list
```

Las dos HIGH deben aparecer `COMPATIBLE`. Las LOW aparecerán `INCOMPATIBLE`:
es normal para esta ruta.

**Qué pegar:** la salida completa de `cal run all`, la consulta final
`cal status` y la lista `impedance cal list`. Esta última contiene los ID y GF.
**No vuelvas a calibrar durante las diez medidas de esta resistencia.**

## 4. Hacer cinco medidas con CAL_HIGH_100R

Selecciona la calibración:

```text
cn0349 impedance cal set CAL_HIGH_100R
```

La respuesta correcta es `Calibration selection: NONE` (sin error).

Para hacer la **medida 1**, ejecuta:

```text
cn0349 impedance start
```

Espera estos dos mensajes:

```text
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
```

Después ejecuta:

```text
cn0349 impedance result
```

**Qué pegar:** el comando `start` y toda su salida, seguido de `result` y toda
su salida. Encabézalo con «2k2 / HIGH_100R / medida 1».

Repite **start → esperar → result → copiar** para las medidas **2, 3, 4 y 5**.
Copia cada resultado antes del siguiente start. No pegues cinco start seguidos;
consultar result cinco veces tampoco genera cinco medidas.

Si hay un error o no aparece COMPLETE, guarda la salida y revisémosla antes
de seguir. Un resultado válido anterior no cuenta como una medida nueva.

## 5. Hacer cinco medidas con CAL_HIGH_1K

**Sin apagar, cambiar la resistencia ni recalibrar**, ejecuta:

```text
cn0349 impedance cal set CAL_HIGH_1K
```

Debe responder `Calibration selection: NONE`.
Haz otras cinco medidas usando exactamente el ciclo del paso 4:
**start → esperar COMPLETE y standby → result → copiar**.

**Qué pegar:** las cinco salidas completas de start y result, encabezadas
«2k2 / HIGH_1K / medida 1», «medida 2»… hasta «medida 5».

## 6. Terminar esta resistencia y repetir con la otra

Después de las diez medidas, ejecuta uno a uno:

```text
cn0349 diag
ad5934 errors
cn0349 impedance cal list
```

**Qué pegar:** los comandos y sus respuestas completas como «Diagnóstico final 2k2».
Así comprobaremos el estado y que los ID/GF no cambiaron durante las medidas.

Ahora apaga la placa, cambia a la resistencia de **220 Ω** y repite los pasos
**1 a 6**, sustituyendo «2k2» por «220 Ω» en las etiquetas de los resultados.
Al encender **sí debes volver a ejecutar cal run all**: al apagar se pierden los GF.

Al terminar tendrás cinco medidas por cada fila del registro siguiente.

## 7. Pegar resultados y comentarios

### Resistencia 2k2 — preparación

Valor del multímetro confirmado para este arranque: **2150 Ω**.

```text
R multímetro: 2150 ohms.
ATLANTES CN-0349
Firmware: 0.9.0
Hito: 2C.1
I2C SDA: GPIO4
I2C SCL: GPIO5
I2C frecuencia configurada: 100000 Hz
I2C timeout configurado: 25 ms
ACK en direccion configurada para AD5934: 0x0D (no identifica el chip).
ADG715 en direccion configurada 0x48: ACK (sin modificar el estado).
Sistema preparado. Escriba help para ver los comandos.

cn0349 recover
Recuperacion controlada; preset ADG715 conservado.
Recuperacion completa: AD5934 en standby verificado; mascara ADG715 conservada y leida.
Manager recovery: COMPLETE
cn0349 diag
Diagnostico integrado no destructivo:
AD5934 0xD: ACK
ADG715 0x48: ACK
AD5934 control seguro: si
Configuracion relevante valida: si
Status: 0x70
ADG715 mask readback: 0x00; preset conocido: si
Resultado diagnostico: READY
La lectura del ADG715 no verifica electricamente la ruta analogica.
Manager diagnostic: READY
cn0349 impedance config
MEASURE_PT100 / 0x41 (RTD), RFB=100 ohm; 2000 Hz exclusivamente.
Calibracion seleccionada: CAL_HIGH_100R
Perfil actual Hz/MCLK/Range/PGA/settling: 2000 / 1000000 / 1 / 1 / 10
Z_aparente=1/(GF*M); sin correccion de ruta ni temperatura.
No detecta fiablemente saturacion/senal insuficiente; validez numerica no demuestra exactitud.

```

**Calibración de este arranque: COMPLETE, error NONE.** Las veinte adquisiciones
internas reportan D1 válido y standby. Ambas HIGH son compatibles para medir Pt100.

| Calibración | ID | GF de este arranque |
| --- | --- | --- |
| CAL_HIGH_100R | 1 | 0.000001220372 |
| CAL_HIGH_1K | 2 | 0.000001164125 |

<details>
<summary>Salida completa de calibración (desplegar para consultar)</summary>

```text
cn0349 cal run all
Solicitado CAL_HIGH_100R 0x09; confirmado por lectura del registro ADG715.
Esto no verifica electricamente la ruta analogica.
Adquisicion DFT unica iniciada; configurando registros verificados.
Calibracion secuencial iniciada; cuatro presets, cinco muestras cada uno.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_HIGH_100R (0x09)
RFB nominal   : 100 ohm
RCAL nominal  : 100 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -3374
IMAG          : -7470
Magnitude     : 8196.632
Elapsed       : 199 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_HIGH_100R 0x09.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_HIGH_100R (0x09)
RFB nominal   : 100 ohm
RCAL nominal  : 100 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -3378
IMAG          : -7466
Magnitude     : 8194.635
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_HIGH_100R 0x09.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_HIGH_100R (0x09)
RFB nominal   : 100 ohm
RCAL nominal  : 100 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -3380
IMAG          : -7464
Magnitude     : 8193.638
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_HIGH_100R 0x09.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_HIGH_100R (0x09)
RFB nominal   : 100 ohm
RCAL nominal  : 100 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -3381
IMAG          : -7463
Magnitude     : 8193.139
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_HIGH_100R 0x09.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_HIGH_100R (0x09)
RFB nominal   : 100 ohm
RCAL nominal  : 100 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -3383
IMAG          : -7462
Magnitude     : 8193.054
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Solicitado CAL_HIGH_1K 0x11; confirmado por lectura del registro ADG715.
Esto no verifica electricamente la ruta analogica.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_HIGH_1K (0x11)
RFB nominal   : 100 ohm
RCAL nominal  : 1000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -359
IMAG          : -781
Magnitude     : 859.559
Elapsed       : 183 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_HIGH_1K 0x11.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_HIGH_1K (0x11)
RFB nominal   : 100 ohm
RCAL nominal  : 1000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -359
IMAG          : -781
Magnitude     : 859.559
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_HIGH_1K 0x11.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_HIGH_1K (0x11)
RFB nominal   : 100 ohm
RCAL nominal  : 1000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -359
IMAG          : -780
Magnitude     : 858.651
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_HIGH_1K 0x11.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_HIGH_1K (0x11)
RFB nominal   : 100 ohm
RCAL nominal  : 1000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -359
IMAG          : -780
Magnitude     : 858.651
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_HIGH_1K 0x11.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_HIGH_1K (0x11)
RFB nominal   : 100 ohm
RCAL nominal  : 1000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -359
IMAG          : -780
Magnitude     : 858.651
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Solicitado CAL_LOW_1K 0x12; confirmado por lectura del registro ADG715.
Esto no verifica electricamente la ruta analogica.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_LOW_1K (0x12)
RFB nominal   : 1000 ohm
RCAL nominal  : 1000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -3413
IMAG          : -7510
Magnitude     : 8249.162
Elapsed       : 183 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_LOW_1K 0x12.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_LOW_1K (0x12)
RFB nominal   : 1000 ohm
RCAL nominal  : 1000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -3414
IMAG          : -7509
Magnitude     : 8248.665
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_LOW_1K 0x12.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_LOW_1K (0x12)
RFB nominal   : 1000 ohm
RCAL nominal  : 1000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -3414
IMAG          : -7508
Magnitude     : 8247.755
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_LOW_1K 0x12.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_LOW_1K (0x12)
RFB nominal   : 1000 ohm
RCAL nominal  : 1000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -3415
IMAG          : -7507
Magnitude     : 8247.259
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_LOW_1K 0x12.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_LOW_1K (0x12)
RFB nominal   : 1000 ohm
RCAL nominal  : 1000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -3415
IMAG          : -7507
Magnitude     : 8247.259
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Solicitado CAL_LOW_10K 0x22; confirmado por lectura del registro ADG715.
Esto no verifica electricamente la ruta analogica.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_LOW_10K (0x22)
RFB nominal   : 1000 ohm
RCAL nominal  : 10000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -351
IMAG          : -762
Magnitude     : 838.955
Elapsed       : 183 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_LOW_10K 0x22.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_LOW_10K (0x22)
RFB nominal   : 1000 ohm
RCAL nominal  : 10000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -352
IMAG          : -761
Magnitude     : 838.466
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_LOW_10K 0x22.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_LOW_10K (0x22)
RFB nominal   : 1000 ohm
RCAL nominal  : 10000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -351
IMAG          : -762
Magnitude     : 838.955
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_LOW_10K 0x22.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_LOW_10K (0x22)
RFB nominal   : 1000 ohm
RCAL nominal  : 10000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -351
IMAG          : -762
Magnitude     : 838.955
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
ADG715 ya confirma CAL_LOW_10K 0x22.
Adquisicion DFT unica iniciada; configurando registros verificados.
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : CAL_LOW_10K (0x22)
RFB nominal   : 1000 ohm
RCAL nominal  : 10000 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -352
IMAG          : -762
Magnitude     : 839.374
Elapsed       : 186 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
cn0349 cal status
Calibration state: COMPLETE; preset: CAL_LOW_10K; sample: 5/5
All sequence: yes
Last calibration error: NONE
cn0349 impedance cal list
CAL_HIGH_100R: COMPATIBLE; ID=1; GF=0.000001220372
CAL_HIGH_1K: COMPATIBLE; ID=2; GF=0.000001164125
CAL_LOW_1K: INCOMPATIBLE RFB=1000 ohm
CAL_LOW_10K: INCOMPATIBLE RFB=1000 ohm
```

</details>

Comprobación de preparación: recovery COMPLETE y diagnóstico READY, con standby
verificado. Perfil solicitado: 2000 Hz, MCLK 1 MHz, Range 1, PGA x1 y settling 10.
La máscara física observada es 0x00 (ALL_OFF); la configuración muestra 0x41 como
ruta prevista para la medida. La selección efectiva se verificará al iniciar DFT.
Calibración del arranque actual: recibida y COMPLETE. Medidas externas nuevas: 20/20; series A, B, C y D completas (5/5 cada una). Diagnósticos finales de ambas cargas recibidos y correctos.
Las veinte adquisiciones de calibración interna no cuentan como medidas de la resistencia externa.

GF recibidos antes del arranque recién pegado (referencia histórica; no asignarlos
a este arranque hasta recibir su nueva calibración):

```text
cn0349 impedance cal list
CAL_HIGH_100R: COMPATIBLE; ID=1; GF=0.000001220520
CAL_HIGH_1K: COMPATIBLE; ID=2; GF=0.000001164618
CAL_LOW_1K: INCOMPATIBLE RFB=1000 ohm
CAL_LOW_10K: INCOMPATIBLE RFB=1000 ohm
```

Si vuelves a arrancar o calibrar, pega la nueva lista identificándola;
no mezcles medidas de arranques distintos en una misma serie de cinco.

### Resistencia 2k2 — cinco medidas HIGH_100R

| Medida | Z aparente Ω | REAL / IMAG | Magnitud DFT | Fase corregida ° | Estado |
| --- | --- | --- | --- | --- | --- |
| C1 | 2019.328491 | −171 / −368 | 405.789368 | −0.568108 | COMPLETE / VALID / NONE |
| C2 | 2023.850464 | −171 / −367 | 404.882690 | −0.627739 | COMPLETE / VALID / NONE |
| C3 | 2019.328491 | −171 / −368 | 405.789368 | −0.568108 | COMPLETE / VALID / NONE |
| C4 | 2019.328491 | −171 / −368 | 405.789368 | −0.568108 | COMPLETE / VALID / NONE |
| C5 | 2019.328491 | −171 / −368 | 405.789368 | −0.568108 | COMPLETE / VALID / NONE |

<details>
<summary>C1 — salida completa</summary>

```text
cn0349 impedance start
Solicitado RTD 0x41; confirmado por lectura del registro ADG715.
Esto no verifica electricamente la ruta analogica.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -171
IMAG          : -368
Magnitude     : 405.789
Elapsed       : 184 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_100R / 0x9 / 1
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -171 / -368
DFT magnitude (code): 405.789368
DFT/calibration phase deg: -114.923065 / -114.354958
Phase corrected instrumentally deg (not validated physical phase): -0.568108
Gain Factor (1/(ohm*DFT code)): 0.000001220372
Apparent impedance ohm: 2019.328491
Elapsed ms: 226
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>C2 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -171
IMAG          : -367
Magnitude     : 404.883
Elapsed       : 176 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_100R / 0x9 / 1
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -171 / -367
DFT magnitude (code): 404.882690
DFT/calibration phase deg: -114.982697 / -114.354958
Phase corrected instrumentally deg (not validated physical phase): -0.627739
Gain Factor (1/(ohm*DFT code)): 0.000001220372
Apparent impedance ohm: 2023.850464
Elapsed ms: 218
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>C3 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -171
IMAG          : -368
Magnitude     : 405.789
Elapsed       : 176 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_100R / 0x9 / 1
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -171 / -368
DFT magnitude (code): 405.789368
DFT/calibration phase deg: -114.923065 / -114.354958
Phase corrected instrumentally deg (not validated physical phase): -0.568108
Gain Factor (1/(ohm*DFT code)): 0.000001220372
Apparent impedance ohm: 2019.328491
Elapsed ms: 218
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>C4 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -171
IMAG          : -368
Magnitude     : 405.789
Elapsed       : 176 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_100R / 0x9 / 1
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -171 / -368
DFT magnitude (code): 405.789368
DFT/calibration phase deg: -114.923065 / -114.354958
Phase corrected instrumentally deg (not validated physical phase): -0.568108
Gain Factor (1/(ohm*DFT code)): 0.000001220372
Apparent impedance ohm: 2019.328491
Elapsed ms: 218
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>C5 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -171
IMAG          : -368
Magnitude     : 405.789
Elapsed       : 172 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_100R / 0x9 / 1
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -171 / -368
DFT magnitude (code): 405.789368
DFT/calibration phase deg: -114.923065 / -114.354958
Phase corrected instrumentally deg (not validated physical phase): -0.568108
Gain Factor (1/(ohm*DFT code)): 0.000001220372
Apparent impedance ohm: 2019.328491
Elapsed ms: 214
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

Comentarios: C1 recibida, con ID 1 y GF 0.000001220372 coincidentes con la
calibración de este arranque; D1 válido y standby reportados. Frente a 2150 Ω,
la diferencia de esta muestra es −130.671509 Ω (−6.077745 %).
C2 recibida: 2023.850464 Ω, duración Manager 218 ms, misma configuración,
ID y GF que C1; COMPLETE/VALID/NONE y standby reportados.
C3 recibida: 2019.328491 Ω, duración Manager 218 ms, misma configuración, ID y GF.
DFT y Z coinciden con C1 a la resolución reportada; COMPLETE/VALID/NONE y standby.
C4 recibida: 2019.328491 Ω, duración Manager 218 ms, misma configuración, ID y GF.
La salida coincide con C3 a la resolución reportada; COMPLETE/VALID/NONE y standby.
C5 recibida: 2019.328491 Ω, duración Manager 214 ms, misma configuración, ID y GF;
COMPLETE/VALID/NONE y standby reportados. Serie C completa: 5/5 válidas.

**Resumen de la serie C (lectura indicada por multímetro: 2150 Ω):**

| Indicador | Resultado |
| --- | --- |
| Media Z aparente | 2020.232886 Ω |
| Desviación estándar muestral (n−1) | 2.022288 Ω |
| Variación relativa (s/media) | 0.100102 % |
| Mínimo / máximo | 2019.328491 / 2023.850464 Ω |
| Recorrido | 4.521973 Ω |
| Diferencia de la media con lectura indicada por multímetro | -129.767114 Ω (-6.035680 %) |
| Media / desviación muestral magnitud DFT | 405.608032 / 0.405479 |

Las cinco muestras conservaron HIGH_100R, ID 1 y GF 0.000001220372, con ruta
0x41 y perfil 2000 Hz / MCLK 1 MHz / Range 1 / PGA x1 / settling 10.
Cuatro DFT coinciden a la resolución reportada; C2 difiere en un código de IMAG
(−367 frente a −368). Esa cuantización se refleja en dos valores de Z aparentes.
La dispersión observada es pequeña frente a la desviación de aproximadamente
−6.04 % respecto a la indicación del multímetro. Esto describe repetibilidad de estas cinco
adquisiciones, no exactitud absoluta ni causa física de la desviación. Se
conservan todas las muestras y no se modifica GF. Pendiente la serie D con HIGH_1K.

### Resistencia 2k2 — cinco medidas HIGH_1K

| Medida | Z aparente Ω | REAL / IMAG | Magnitud DFT | Fase corregida ° | Estado |
| --- | --- | --- | --- | --- | --- |
| D1 | 2116.896484 | −171 / −368 | 405.789368 | −0.219650 | COMPLETE / VALID / NONE |
| D2 | 2116.896484 | −171 / −368 | 405.789368 | −0.219650 | COMPLETE / VALID / NONE |
| D3 | 2121.636963 | −171 / −367 | 404.882690 | −0.279282 | COMPLETE / VALID / NONE |
| D4 | 2121.636963 | −171 / −367 | 404.882690 | −0.279282 | COMPLETE / VALID / NONE |
| D5 | 2116.896484 | −171 / −368 | 405.789368 | −0.219650 | COMPLETE / VALID / NONE |

<details>
<summary>D1 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -171
IMAG          : -368
Magnitude     : 405.789
Elapsed       : 176 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_1K / 0x11 / 2
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -171 / -368
DFT magnitude (code): 405.789368
DFT/calibration phase deg: -114.923065 / -114.703415
Phase corrected instrumentally deg (not validated physical phase): -0.219650
Gain Factor (1/(ohm*DFT code)): 0.000001164125
Apparent impedance ohm: 2116.896484
Elapsed ms: 218
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>D2 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -171
IMAG          : -368
Magnitude     : 405.789
Elapsed       : 172 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impdenace result
Comando CN-0349 no valido. Use cn0349 help.
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_1K / 0x11 / 2
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -171 / -368
DFT magnitude (code): 405.789368
DFT/calibration phase deg: -114.923065 / -114.703415
Phase corrected instrumentally deg (not validated physical phase): -0.219650
Gain Factor (1/(ohm*DFT code)): 0.000001164125
Apparent impedance ohm: 2116.896484
Elapsed ms: 214
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>D3 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -171
IMAG          : -367
Magnitude     : 404.883
Elapsed       : 176 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_1K / 0x11 / 2
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -171 / -367
DFT magnitude (code): 404.882690
DFT/calibration phase deg: -114.982697 / -114.703415
Phase corrected instrumentally deg (not validated physical phase): -0.279282
Gain Factor (1/(ohm*DFT code)): 0.000001164125
Apparent impedance ohm: 2121.636963
Elapsed ms: 218
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>D4 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -171
IMAG          : -367
Magnitude     : 404.883
Elapsed       : 176 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_1K / 0x11 / 2
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -171 / -367
DFT magnitude (code): 404.882690
DFT/calibration phase deg: -114.982697 / -114.703415
Phase corrected instrumentally deg (not validated physical phase): -0.279282
Gain Factor (1/(ohm*DFT code)): 0.000001164125
Apparent impedance ohm: 2121.636963
Elapsed ms: 218
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>D5 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -171
IMAG          : -368
Magnitude     : 405.789
Elapsed       : 176 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_1K / 0x11 / 2
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -171 / -368
DFT magnitude (code): 405.789368
DFT/calibration phase deg: -114.923065 / -114.703415
Phase corrected instrumentally deg (not validated physical phase): -0.219650
Gain Factor (1/(ohm*DFT code)): 0.000001164125
Apparent impedance ohm: 2116.896484
Elapsed ms: 218
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

Comentarios: D1 recibida con HIGH_1K, ID 2 y GF 0.000001164125 coincidentes
con la calibración de este arranque. Ruta 0x41, perfil sin cambios,
COMPLETE/VALID/NONE y standby reportados; duración Manager 218 ms.
D2 recibida: 2116.896484 Ω, duración Manager 214 ms, misma configuración,
ID y GF; COMPLETE/VALID/NONE y standby reportados. Coincide con la DFT y Z de D1
a la resolución reportada. Se escribió `cn0349 impdenace result`, rechazado por
el parser, y después se consultó correctamente `cn0349 impedance result`.
No hubo otro start: la corrección de escritura no repite ni invalida la adquisición.
D2 cuenta como una sola medida.
D3 recibida: 2121.636963 Ω, duración Manager 218 ms, misma configuración, ID y GF;
COMPLETE/VALID/NONE y standby reportados. IMAG pasa de −368 a −367 respecto
a D1/D2, con magnitud 404.882690 y fase corregida −0.279282°.
D4 recibida: 2121.636963 Ω, duración Manager 218 ms, misma configuración, ID y GF;
la salida coincide con D3 a la resolución reportada. COMPLETE/VALID/NONE y standby.
D5 recibida: 2116.896484 Ω, duración Manager 218 ms, misma configuración, ID y GF;
COMPLETE/VALID/NONE y standby reportados. Serie D completa: 5/5 válidas.

**Resumen de la serie D (lectura indicada por multímetro: 2150 Ω):**

| Indicador | Resultado |
| --- | --- |
| Media Z aparente | 2118.792676 Ω |
| Desviación estándar muestral (n−1) | 2.596467 Ω |
| Variación relativa (s/media) | 0.122545 % |
| Mínimo / máximo | 2116.896484 / 2121.636963 Ω |
| Recorrido | 4.740479 Ω |
| Diferencia de la media con lectura indicada por multímetro | -31.207324 Ω (-1.451503 %) |
| Media / desviación muestral magnitud DFT | 405.426697 / 0.496608 |

Las cinco muestras conservaron HIGH_1K, ID 2 y GF 0.000001164125, ruta 0x41
y perfil 2000 Hz / MCLK 1 MHz / Range 1 / PGA x1 / settling 10.
Se observan dos pares DFT: −171/−368 (tres muestras) y −171/−367 (dos muestras).
La dispersión observada es pequeña frente a la diferencia de la media con el
multímetro. No se infiere exactitud absoluta ni ausencia de ruido por debajo
de la resolución DFT. No se cambia GF ni se descartan muestras.

**Comparación de la carga 2k2:** HIGH_100R presenta media 2020.232886 Ω,
s=2.022288 Ω y diferencia −6.035680 %; HIGH_1K presenta media 2118.792676 Ω,
s=2.596467 Ω y diferencia -1.451503 %. HIGH_1K queda más cerca de los 2150 Ω
reportados en este ensayo. Las dos series usaron los coeficientes del mismo
arranque; sus diferencias no establecen una regla universal de selección ni
justifican promediar GF. Diagnóstico final recibido y correcto; pendiente ensayo de la carga 220 Ω.

### Resistencia 2k2 — diagnóstico final

```text
cn0349 diag
Diagnostico integrado no destructivo:
AD5934 0xD: ACK
ADG715 0x48: ACK
AD5934 control seguro: si
Configuracion relevante valida: si
Status: 0x72
ADG715 mask readback: 0x41; preset conocido: si
Resultado diagnostico: READY
La lectura del ADG715 no verifica electricamente la ruta analogica.
Manager diagnostic: READY
ad5934 errors
Error activo: NONE
Ultimo error: NONE
Counters:
  BUSY: 0
  INVALID_ARGUMENT: 0
  INVALID_STATE: 0
  I2C_NACK: 0
  I2C_TIMEOUT: 0
  I2C_ERROR: 0
  REGISTER_VERIFY_FAILED: 0
  ADG715_VERIFY_FAILED: 0
  DFT_TIMEOUT: 0
  SWEEP_TIMEOUT: 0
  DATA_INVALID: 0
  RECOVERY_FAILED: 0
cn0349 impedance cal list
CAL_HIGH_100R: COMPATIBLE; ID=1; GF=0.000001220372
CAL_HIGH_1K: COMPATIBLE; ID=2; GF=0.000001164125
CAL_LOW_1K: INCOMPATIBLE RFB=1000 ohm
CAL_LOW_10K: INCOMPATIBLE RFB=1000 ohm
```

Comentarios de cierre: diagnóstico READY, control seguro y configuración válida;
Status 0x72 y máscara RTD 0x41 leídos. Error activo/último NONE y todos los
contadores reportados a cero. Los ID y GF de las dos HIGH coinciden con los
registrados antes de C1: no cambiaron durante las diez medidas. La incompatibilidad
LOW por RFB es esperada. Queda completada la toma de datos de 2k2 (C/D) y su
diagnóstico posterior; pendiente repetir el procedimiento con 220 Ω.

### Resistencia 220 Ω — preparación

Valor del multímetro confirmado para esta prueba: **215 Ω** (nominal 220 Ω).

```text
R 220R teorico - medida multimetro 215ohm.
cn0349 recover
Recuperacion controlada; preset ADG715 conservado.
Recuperacion completa: AD5934 en standby verificado; mascara ADG715 conservada y leida.
Manager recovery: COMPLETE
cn0349 diag
Diagnostico integrado no destructivo:
AD5934 0xD: ACK
ADG715 0x48: ACK
AD5934 control seguro: si
Configuracion relevante valida: si
Status: 0x70
ADG715 mask readback: 0x00; preset conocido: si
Resultado diagnostico: READY
La lectura del ADG715 no verifica electricamente la ruta analogica.
Manager diagnostic: READY
cn0349 impedance config
MEASURE_PT100 / 0x41 (RTD), RFB=100 ohm; 2000 Hz exclusivamente.
Calibracion seleccionada: CAL_HIGH_100R
Perfil actual Hz/MCLK/Range/PGA/settling: 2000 / 1000000 / 1 / 1 / 10
Z_aparente=1/(GF*M); sin correccion de ruta ni temperatura.
No detecta fiablemente saturacion/senal insuficiente; validez numerica no demuestra exactitud.

cn0349 impedance cal list
CAL_HIGH_100R: COMPATIBLE; ID=1; GF=0.000001220810
CAL_HIGH_1K: COMPATIBLE; ID=2; GF=0.000001164410
CAL_LOW_1K: INCOMPATIBLE RFB=1000 ohm
CAL_LOW_10K: INCOMPATIBLE RFB=1000 ohm

cn0349 cal status
Calibration state: COMPLETE; preset: CAL_LOW_10K; sample: 5/5
All sequence: yes
Last calibration error: NONE

[Pendiente salida de arranque y transcript de cal run all; no aportados todavía.]
```

Comentarios de preparación: referencia confirmada 215 Ω; recovery COMPLETE,
diagnóstico READY, control seguro y perfil 2000 Hz / MCLK 1 MHz / Range 1 /
PGA x1 / settling 10. Máscara física inicial 0x00; 0x41 es la ruta prevista
para medir. La lista recibida confirma ambas HIGH disponibles y compatibles:
HIGH_100R ID 1 / GF 0.000001220810; HIGH_1K ID 2 / GF 0.000001164410.
Estos son los coeficientes de la prueba de 220 Ω, distintos de los de 2k2.
`cal status` recibido: COMPLETE, último preset CAL_LOW_10K, muestra 5/5,
secuencia all confirmada y error NONE. Preparación suficiente para comenzar A1
con HIGH_100R; falta únicamente el transcript completo de calibración como
registro detallado, sin impedir la toma de medidas.

### Resistencia 220 Ω — cinco medidas HIGH_100R

| Medida | Z aparente Ω | REAL / IMAG | Magnitud DFT | Fase corregida ° | Estado |
| --- | --- | --- | --- | --- | --- |
| A1 | 210.447388 | −1605 / −3546 | 3892.318115 | +0.065437 | COMPLETE / VALID / NONE |
| A2 | 210.398132 | −1605 / −3547 | 3893.229248 | +0.071503 | COMPLETE / VALID / NONE |
| A3 | 210.469666 | −1604 / −3546 | 3891.906006 | +0.078850 | COMPLETE / VALID / NONE |
| A4 | 210.398132 | −1605 / −3547 | 3893.229248 | +0.071503 | COMPLETE / VALID / NONE |
| A5 | 210.398132 | −1605 / −3547 | 3893.229248 | +0.071503 | COMPLETE / VALID / NONE |

<details>
<summary>A1 — salida completa</summary>

```text
cn0349 impedance cal set CAL_HIGH_100R
Calibration selection: NONE
cn0349 impedance start
Solicitado RTD 0x41; confirmado por lectura del registro ADG715.
Esto no verifica electricamente la ruta analogica.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -1605
IMAG          : -3546
Magnitude     : 3892.318
Elapsed       : 184 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_100R / 0x9 / 1
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -1605 / -3546
DFT magnitude (code): 3892.318115
DFT/calibration phase deg: -114.352585 / -114.418022
Phase corrected instrumentally deg (not validated physical phase): 0.065437
Gain Factor (1/(ohm*DFT code)): 0.000001220810
Apparent impedance ohm: 210.447388
Elapsed ms: 226
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>A2 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -1605
IMAG          : -3547
Magnitude     : 3893.229
Elapsed       : 172 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_100R / 0x9 / 1
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -1605 / -3547
DFT magnitude (code): 3893.229248
DFT/calibration phase deg: -114.346519 / -114.418022
Phase corrected instrumentally deg (not validated physical phase): 0.071503
Gain Factor (1/(ohm*DFT code)): 0.000001220810
Apparent impedance ohm: 210.398132
Elapsed ms: 214
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>A3 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -1604
IMAG          : -3546
Magnitude     : 3891.906
Elapsed       : 172 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_100R / 0x9 / 1
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -1604 / -3546
DFT magnitude (code): 3891.906006
DFT/calibration phase deg: -114.339172 / -114.418022
Phase corrected instrumentally deg (not validated physical phase): 0.078850
Gain Factor (1/(ohm*DFT code)): 0.000001220810
Apparent impedance ohm: 210.469666
Elapsed ms: 214
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>A4 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -1605
IMAG          : -3547
Magnitude     : 3893.229
Elapsed       : 177 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_100R / 0x9 / 1
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -1605 / -3547
DFT magnitude (code): 3893.229248
DFT/calibration phase deg: -114.346519 / -114.418022
Phase corrected instrumentally deg (not validated physical phase): 0.071503
Gain Factor (1/(ohm*DFT code)): 0.000001220810
Apparent impedance ohm: 210.398132
Elapsed ms: 219
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>A5 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -1605
IMAG          : -3547
Magnitude     : 3893.229
Elapsed       : 172 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_100R / 0x9 / 1
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -1605 / -3547
DFT magnitude (code): 3893.229248
DFT/calibration phase deg: -114.346519 / -114.418022
Phase corrected instrumentally deg (not validated physical phase): 0.071503
Gain Factor (1/(ohm*DFT code)): 0.000001220810
Apparent impedance ohm: 210.398132
Elapsed ms: 214
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

Comentarios: A1 recibida con HIGH_100R, ID 1 y GF 0.000001220810 coincidentes
con la calibración de esta carga. Referencia multímetro 215 Ω; ruta 0x41,
perfil 2000 Hz / MCLK 1 MHz / Range 1 / PGA x1 / settling 10;
COMPLETE/VALID/NONE y standby reportados. Duración Manager 226 ms.
A2 recibida: 210.398132 Ω, duración Manager 214 ms; mismo ID, GF y configuración,
COMPLETE/VALID/NONE y standby reportados. IMAG −3547 frente a −3546 en A1.
A3 recibida: 210.469666 Ω, REAL −1604 / IMAG −3546, duración Manager 214 ms;
mismo ID, GF y configuración. COMPLETE/VALID/NONE y standby reportados.
A4 recibida: 210.398132 Ω, duración Manager 219 ms, misma configuración, ID y GF;
DFT y Z coinciden con A2 a la resolución reportada. COMPLETE/VALID/NONE y standby.
A5 recibida: 210.398132 Ω, duración Manager 214 ms, misma configuración, ID y GF;
COMPLETE/VALID/NONE y standby reportados. Serie A completa: 5/5 válidas.

**Resumen de la serie A (lectura indicada por multímetro: 215 Ω):**

| Indicador | Resultado |
| --- | --- |
| Media Z aparente | 210.422290 Ω |
| Desviación estándar muestral (n−1) | 0.034004 Ω |
| Variación relativa (s/media) | 0.016160 % |
| Mínimo / máximo | 210.398132 / 210.469666 Ω |
| Recorrido | 0.071534 Ω |
| Diferencia de la media con lectura indicada por multímetro | -4.577710 Ω (-2.129167 %) |
| Media / desviación muestral magnitud DFT | 3892.782373 / 0.629016 |

Las cinco muestras conservaron HIGH_100R, ID 1 y GF 0.000001220810, ruta 0x41
y perfil 2000 Hz / MCLK 1 MHz / Range 1 / PGA x1 / settling 10.
La dispersión de estas cinco adquisiciones es pequeña frente a la diferencia
de aproximadamente −2.13 % respecto a la indicación del multímetro. Eso describe repetibilidad,
no precisión absoluta ni causa física de la discrepancia. Se conservan todas
las muestras y no se modifica GF. Pendiente serie B con HIGH_1K.

### Resistencia 220 Ω — cinco medidas HIGH_1K

| Medida | Z aparente Ω | REAL / IMAG | Magnitud DFT | Fase corregida ° | Estado |
| --- | --- | --- | --- | --- | --- |
| B1 | 220.612457 | −1604 / −3547 | 3892.817139 | +0.435516 | COMPLETE / VALID / NONE |
| B2 | 220.560852 | −1604 / −3548 | 3893.728271 | +0.441582 | COMPLETE / VALID / NONE |
| B3 | 220.560852 | −1604 / −3548 | 3893.728271 | +0.441582 | COMPLETE / VALID / NONE |
| B4 | 220.560852 | −1604 / −3548 | 3893.728271 | +0.441582 | COMPLETE / VALID / NONE |
| B5 | 220.560852 | −1604 / −3548 | 3893.728271 | +0.441582 | COMPLETE / VALID / NONE |

<details>
<summary>B1 — salida completa</summary>

```text
cn0349 impedance cal set CAL_HIGH_1K
Calibration selection: NONE
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -1604
IMAG          : -3547
Magnitude     : 3892.817
Elapsed       : 177 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_1K / 0x11 / 2
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -1604 / -3547
DFT magnitude (code): 3892.817139
DFT/calibration phase deg: -114.333107 / -114.768623
Phase corrected instrumentally deg (not validated physical phase): 0.435516
Gain Factor (1/(ohm*DFT code)): 0.000001164410
Apparent impedance ohm: 220.612457
Elapsed ms: 219
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>B2 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -1604
IMAG          : -3548
Magnitude     : 3893.728
Elapsed       : 172 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_1K / 0x11 / 2
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -1604 / -3548
DFT magnitude (code): 3893.728271
DFT/calibration phase deg: -114.327042 / -114.768623
Phase corrected instrumentally deg (not validated physical phase): 0.441582
Gain Factor (1/(ohm*DFT code)): 0.000001164410
Apparent impedance ohm: 220.560852
Elapsed ms: 214
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>B3 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -1604
IMAG          : -3548
Magnitude     : 3893.728
Elapsed       : 176 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_1K / 0x11 / 2
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -1604 / -3548
DFT magnitude (code): 3893.728271
DFT/calibration phase deg: -114.327042 / -114.768623
Phase corrected instrumentally deg (not validated physical phase): 0.441582
Gain Factor (1/(ohm*DFT code)): 0.000001164410
Apparent impedance ohm: 220.560852
Elapsed ms: 218
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>B4 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -1604
IMAG          : -3548
Magnitude     : 3893.728
Elapsed       : 172 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_1K / 0x11 / 2
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -1604 / -3548
DFT magnitude (code): 3893.728271
DFT/calibration phase deg: -114.327042 / -114.768623
Phase corrected instrumentally deg (not validated physical phase): 0.441582
Gain Factor (1/(ohm*DFT code)): 0.000001164410
Apparent impedance ohm: 220.560852
Elapsed ms: 214
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

<details>
<summary>B5 — salida completa</summary>

```text
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
-------------------------------------
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -1604
IMAG          : -3548
Magnitude     : 3893.728
Elapsed       : 176 ms
Magnitude is a raw DFT code, not ohms.
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_1K / 0x11 / 2
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -1604 / -3548
DFT magnitude (code): 3893.728271
DFT/calibration phase deg: -114.327042 / -114.768623
Phase corrected instrumentally deg (not validated physical phase): 0.441582
Gain Factor (1/(ohm*DFT code)): 0.000001164410
Apparent impedance ohm: 220.560852
Elapsed ms: 218
Hardware error (historical on failure): NONE
Sin correccion serie; no equivale a resistencia externa validada. Saturacion/senal insuficiente no detectadas fiablemente.
```

</details>

Comentarios: B1 recibida con HIGH_1K, ID 2 y GF 0.000001164410 coincidentes
con la calibración de esta carga. Referencia multímetro 215 Ω, ruta 0x41,
perfil 2000 Hz / MCLK 1 MHz / Range 1 / PGA x1 / settling 10;
COMPLETE/VALID/NONE y standby reportados. Duración Manager 219 ms.
B2 recibida: 220.560852 Ω, duración Manager 214 ms; mismo ID, GF y configuración,
COMPLETE/VALID/NONE y standby reportados. IMAG −3548 frente a −3547 en B1.
B3 recibida: 220.560852 Ω, duración Manager 218 ms; mismo ID, GF y configuración.
DFT y Z coinciden con B2 a la resolución reportada; COMPLETE/VALID/NONE y standby.
B4 recibida: 220.560852 Ω, duración Manager 214 ms; mismo ID, GF y configuración.
La salida coincide con B2 a la resolución reportada; COMPLETE/VALID/NONE y standby.
B5 recibida: 220.560852 Ω, duración Manager 218 ms, misma configuración, ID y GF;
COMPLETE/VALID/NONE y standby reportados. Serie B completa: 5/5 válidas.

**Resumen de la serie B (lectura indicada por multímetro: 215 Ω):**

| Indicador | Resultado |
| --- | --- |
| Media Z aparente | 220.571173 Ω |
| Desviación estándar muestral (n−1) | 0.023078 Ω |
| Variación relativa (s/media) | 0.010463 % |
| Mínimo / máximo | 220.560852 / 220.612457 Ω |
| Recorrido | 0.051605 Ω |
| Diferencia de la media con lectura indicada por multímetro | 5.571173 Ω (2.591243 %) |
| Media / desviación muestral magnitud DFT | 3893.546045 / 0.407471 |

Las cinco muestras conservaron HIGH_1K, ID 2 y GF 0.000001164410, ruta 0x41
y perfil 2000 Hz / MCLK 1 MHz / Range 1 / PGA x1 / settling 10. Cuatro DFT
coinciden a la resolución reportada; B1 difiere en un código de IMAG.
La dispersión observada es pequeña frente a la desviación de la media respecto
al multímetro. Se conservan las cinco muestras y no se modifica GF.
Diagnóstico posterior de la carga 220 Ω recibido: READY, errores a cero e ID/GF sin cambios.

### Resistencia 220 Ω — diagnóstico final

```text
cn0349 diag
Diagnostico integrado no destructivo:
AD5934 0xD: ACK
ADG715 0x48: ACK
AD5934 control seguro: si
Configuracion relevante valida: si
Status: 0x72
ADG715 mask readback: 0x41; preset conocido: si
Resultado diagnostico: READY
La lectura del ADG715 no verifica electricamente la ruta analogica.
Manager diagnostic: READY
ad5934 errors
Error activo: NONE
Ultimo error: NONE
Counters:
  BUSY: 0
  INVALID_ARGUMENT: 0
  INVALID_STATE: 0
  I2C_NACK: 0
  I2C_TIMEOUT: 0
  I2C_ERROR: 0
  REGISTER_VERIFY_FAILED: 0
  ADG715_VERIFY_FAILED: 0
  DFT_TIMEOUT: 0
  SWEEP_TIMEOUT: 0
  DATA_INVALID: 0
  RECOVERY_FAILED: 0
cn0349 impedance cal list
CAL_HIGH_100R: COMPATIBLE; ID=1; GF=0.000001220810
CAL_HIGH_1K: COMPATIBLE; ID=2; GF=0.000001164410
CAL_LOW_1K: INCOMPATIBLE RFB=1000 ohm
CAL_LOW_10K: INCOMPATIBLE RFB=1000 ohm
```

Comentarios de cierre: diagnóstico READY, control seguro, configuración válida,
Status 0x72 y máscara RTD 0x41. Error activo/último NONE y todos los contadores
reportados a cero. HIGH_100R ID 1 / GF 0.000001220810 y HIGH_1K ID 2 /
GF 0.000001164410 coinciden con los registrados antes de A1. LOW incompatible
por RFB es esperado. Completadas las diez medidas de esta carga y su diagnóstico.

### Resumen (lo completaremos al recibir las medidas)

| Resistencia | Calibración | Medidas recibidas | Media Ω | Dispersión Ω | Diferencia con indicación DMM % |
| --- | --- | --- | --- | --- | --- |
| 2k2 | HIGH_100R | 5/5 | 2020.232886 | 2.022288 | -6.035680 |
| 2k2 | HIGH_1K | 5/5 | 2118.792676 | 2.596467 | -1.451503 |
| 220 Ω | HIGH_100R | 5/5 | 210.422290 | 0.034004 | -2.129167 |
| 220 Ω | HIGH_1K | 5/5 | 220.571173 | 0.023078 | 2.591243 |

Comentarios conjuntos: veinte adquisiciones externas recibidas, todas con
COMPLETE/VALID/NONE y standby reportado. Se completaron medias y desviaciones
muestrales (n−1) sin descartar muestras. Para 215 Ω, HIGH_100R queda más cerca
de la referencia (−2.129167 %) que HIGH_1K (+2.591243 %). Para 2150 Ω,
HIGH_1K queda más cerca (−1.451503 %) que HIGH_100R (−6.035680 %).
La dispersión de cada serie es pequeña frente a su desviación de referencia;
no demuestra exactitud absoluta ni justifica correcciones, promedio de GF o
autorango. Cada carga utilizó las calibraciones de su propio arranque.
Los DFT discretos repetidos no prueban ausencia de ruido por debajo de resolución.
Diagnósticos de 2k2 y 220 Ω recibidos y correctos, con errores a cero e ID/GF conservados en cada arranque.
Las cuatro muestras preliminares están en [el registro previo](hito2c1.md)
y no se cuentan entre estas veinte medidas nuevas. No se ajustará GF para
forzar los valores del multímetro ni se avanzará a 2C.2 con este ensayo.

**Cierre del ensayo — 2026-10-27:** completadas las 20 adquisiciones externas
(5 por combinación) y los diagnósticos posteriores de ambas cargas. Todas las
adquisiciones reportaron COMPLETE/VALID/NONE y standby; las calibraciones HIGH
se conservaron dentro de cada prueba. Los resultados y la estadística quedan
registrados arriba. Se confirma la ejecución y repetibilidad observada de este
ensayo, sin certificar exactitud absoluta ni resolver la causa de las desviaciones.
**Estado del Hito 2C.1: CERRADO para la primera serie de validación física.**
El hito solicitado (compilación, implementación y documentación) y la matriz
física de 20 adquisiciones están completados. Cierre basado en los datos y
transcripciones aportados por el usuario; no se repitió aquí el hardware.

Las resistencias con banda dorada son nominales y la indicación del multímetro
no es una referencia trazable sin conocer instrumento, rango, resolución e
incertidumbre. Las desviaciones calculadas describen comparación con la lectura
del usuario. No convierten el ensayo en certificación metrológica ni validan
una exactitud absoluta. La dispersión pequeña documenta repetibilidad observada
en estas condiciones. No se modificó GF ni se aplicaron correcciones empíricas.
El pinout y la topología completa no se verificaron independientemente contra
el esquema de fabricación. El alcance queda cerrado en Hito 2C.1; no se inicia 2C.2.

## Medida exploratoria del Pt100 de tres hilos (dos hilos conectados)

Fecha: 2026-10-27. Esta medición se realizó después de cerrar la matriz de
resistencias. Se conserva como observación adicional; no forma parte de las
20 adquisiciones de 220 Ω / 2,2 kΩ ni modifica su estadística.

Conexión reportada: sensor Pt100 de tres hilos, usando solo dos hilos entre
PT100-1 y PT100-2 de la ruta RTD `0x41`. No se identificó aquí qué conductores
corresponden a cada extremo del elemento ni se midió la resistencia de los cables.
No se convirtió resistencia a temperatura.

Calibración seleccionada: CAL_HIGH_100R, ID 1, GF 0.000001220810. Se reporta
COMPLETE/VALID/NONE, D1 válido y standby al terminar.

| Dato | Resultado |
| --- | ---: |
| Frecuencia solicitada / DDS efectiva | 2000 / 1999,996582 Hz |
| REAL / IMAG | −3093 / −6859 |
| Magnitud DFT | 7524,129883 |
| Fase DFT / calibración | −114,272537° / −114,418022° |
| Fase corregida instrumentalmente | +0,145485° |
| Impedancia aparente | **108,866821 Ω** |
| Duración Manager | 214 ms |
| Estado / error / standby | COMPLETE / NONE / reportado |

```text
cn0349 impedance cal list
CAL_HIGH_100R: COMPATIBLE; ID=1; GF=0.000001220810
CAL_HIGH_1K: COMPATIBLE; ID=2; GF=0.000001164410
CAL_LOW_1K: INCOMPATIBLE RFB=1000 ohm
CAL_LOW_10K: INCOMPATIBLE RFB=1000 ohm
cn0349 impedance cal set CAL_HIGH_100R
Calibration selection: NONE
cn0349 impedance start
ADG715 ya confirma RTD 0x41.
Adquisicion DFT unica iniciada; configurando registros verificados.
Impedance request: NONE
ATLANTES - AD5934 Single Measurement
ADG715 preset : RTD (0x41)
RFB nominal   : 100 ohm
RCAL nominal  : 0 ohm
MCLK          : 1000000 Hz external (CN-0349 oscillator)
Frequency     : 2000 Hz requested
Excitation    : Range 1 / 2 Vpp nominal
PGA           : x1
Settling      : 10 cycles x1
Status        : VALID (D1)
REAL          : -3093
IMAG          : -6859
Magnitude     : 7524.130
Elapsed       : 172 ms
AD5934 finalizado en standby.
Pt100 impedance: COMPLETE; error=NONE
cn0349 impedance result
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_100R / 0x9 / 1
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
REAL/IMAG: -3093 / -6859
DFT magnitude (code): 7524.129883
DFT/calibration phase deg: -114.272537 / -114.418022
Phase corrected instrumentally deg: +0.145485
Gain Factor: 0.000001220810
Apparent impedance ohm: 108.866821
Elapsed ms: 214
Hardware error: NONE
```


### Segunda lectura Pt100 con HIGH_1K — observación adicional

La consulta de resultado aportada tras seleccionar HIGH_1K reporta otro intento
COMPLETO/VÁLIDO sobre la misma conexión Pt100 de tres hilos usando dos hilos.
La salida no incluye el `start` correspondiente; se conserva como resultado
reportado, sin afirmar desde este fragmento la secuencia completa de control.

| Dato | Resultado |
| --- | ---: |
| Calibración / ID / GF | HIGH_1K / 2 / 0.000001164410 |
| REAL / IMAG / magnitud DFT | −3096 / −6864 / 7529.920898 |
| Fase DFT / calibración | −114,277702° / −114,768623° |
| Fase corregida instrumentalmente | +0,490921° |
| Impedancia aparente | **114,052193 Ω** |
| Estado / error | COMPLETE / VALID / NONE |
| Duración Manager | 218 ms |

```text
cn0349 impedance result
Impedance state: COMPLETE
Attempt error: NONE
Last request error: NONE
Cancel pending: no
Subsystem busy: no
MEASURE_PT100 / mask 0x41 / RFB nominal 100 ohm
State/valid/error: COMPLETE / VALID / NONE
Calibration/mask/ID: CAL_HIGH_1K / 0x11 / 2
Requested/effective/calibration effective Hz: 2000.000 / 1999.996582 / 1999.996460
MCLK/Range/PGA/settling: 1000000 / 1 / 1 / 10
REAL/IMAG: -3096 / -6864
DFT magnitude (code): 7529.920898
DFT/calibration phase deg: -114.277702 / -114.768623
Phase corrected instrumentally deg (not validated physical phase): 0.490921
Gain Factor (1/(ohm*DFT code)): 0.000001164410
Apparent impedance ohm: 114.052193
Elapsed ms: 218
Hardware error (historical on failure): NONE
```

Frente a la lectura HIGH_100R de 108,866821 Ω, el valor aparente cambia
+5,185372 Ω (+4,76 %) al cambiar GF/calibración; magnitud DFT cambia +5,791016
códigos (+0,077 %). Es una segunda observación instrumental, no repetición
independiente ni prueba de exactitud del Pt100. La conexión de dos hilos sigue
incluyendo cable/contactos potencialmente; no se calcula temperatura.

**Interpretación:** 108,866821 Ω es la impedancia aparente de esta ruta y de
esta conexión de dos hilos. Puede incluir resistencia de conductores/contactos
además del elemento Pt100. La cifra por sí sola no permite separar esas
contribuciones ni afirmar la temperatura del sensor. Hace falta confirmar el
esquema de colores/conductores, la resistencia de cables y una referencia térmica
si se desea validar la conversión a temperatura. No aplicar compensación de
cables ni correcciones empíricas con una sola muestra.
````

### Registro original: hito2e1.md

````text
# Hito 2E.1 — Caracterización eléctrica EC

## Configuración y resultados

Las series A–D usan una resistencia externa de **220 Ω nominales** y las series E–H una de **2,2 kΩ nominales**. La ruta EC del CN0349 incluye **R2 = 100 Ω en serie** con la carga externa. No consta una medida actual de estas resistencias con multímetro. Todas las filas registradas terminaron en `COMPLETE / VALID / NONE`, sin error de hardware y con standby verificado. Frecuencia solicitada: 2000 Hz; efectiva: 1999.996582 Hz; efectiva de calibración: 1999.996460 Hz. MCLK externo: 1 MHz; Range 1; PGA ×1; settling: 10 ciclos.

| Serie | Resistencia nominal | Ruta EC | RFB | Calibración | ID | GF | Muestras |
| --- | ---: | --- | ---: | --- | ---: | ---: | ---: |
| A | 220 Ω | HIGH `0x81` | 100 Ω | `CAL_HIGH_100R` | 1 | 0.000001221432 | 5/5 |
| B | 220 Ω | HIGH `0x81` | 100 Ω | `CAL_HIGH_1K` | 2 | 0.000001164789 | 5/5 |
| C | 220 Ω | LOW `0x82` | 1000 Ω | `CAL_LOW_1K` | 3 | 0.000000121298 | 5/5 |
| D | 220 Ω | LOW `0x82` | 1000 Ω | `CAL_LOW_10K` | 4 | 0.000000119253 | 5/5 |
| E | 2,2 kΩ | HIGH `0x81` | 100 Ω | `CAL_HIGH_100R` | 1 | 0.000001221432 | 5/5 |
| F | 2,2 kΩ | HIGH `0x81` | 100 Ω | `CAL_HIGH_1K` | 2 | 0.000001164789 | 5/5 |
| G | 2,2 kΩ | LOW `0x82` | 1000 Ω | `CAL_LOW_1K` | 3 | 0.000000121298 | 5/5 |
| H | 2,2 kΩ | LOW `0x82` | 1000 Ω | `CAL_LOW_10K` | 4 | 0.000000119253 | 5/5 |

| Medida | REAL | IMAG | Magnitud DFT | Fase corregida (°) | Impedancia aparente (Ω) | Tiempo total (ms) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| A1 | -1102 | -2433 | 2670.934814 | +0.128235 | 306.526031 | 229 |
| A2 | -1101 | -2434 | 2671.433594 | +0.156624 | 306.468781 | 217 |
| A3 | -1101 | -2434 | 2671.433594 | +0.156624 | 306.468781 | 217 |
| A4 | -1101 | -2435 | 2672.344727 | +0.165459 | 306.364319 | 213 |
| A5 | -1101 | -2434 | 2671.433594 | +0.156624 | 306.468781 | 213 |
| B1 | -1101 | -2434 | 2671.433594 | +0.452705 | 321.372192 | 213 |
| B2 | -1101 | -2435 | 2672.344727 | +0.461540 | 321.262634 | 217 |
| B3 | -1101 | -2435 | 2672.344727 | +0.461540 | 321.262634 | 213 |
| B4 | -1101 | -2435 | 2672.344727 | +0.461540 | 321.262634 | 213 |
| B5 | -1101 | -2435 | 2672.344727 | +0.461540 | 321.262634 | 213 |
| C1 | -7801 | -17268 | 18948.335938 | +0.209801 | 435.086731 | 229 |
| C2 | -7799 | -17268 | 18947.511719 | +0.215309 | 435.105682 | 213 |
| C3 | -7800 | -17268 | 18947.923828 | +0.212555 | 435.096191 | 213 |
| C4 | -7799 | -17267 | 18946.601563 | +0.214066 | 435.126556 | 213 |
| C5 | -7801 | -17267 | 18947.423828 | +0.208549 | 435.107666 | 217 |
| D1 | -7800 | -17268 | 18947.923828 | +0.526443 | 442.555023 | 217 |
| D2 | -7798 | -17266 | 18945.277344 | +0.529465 | 442.616882 | 213 |
| D3 | -7800 | -17268 | 18947.923828 | +0.526443 | 442.555023 | 217 |
| D4 | -7799 | -17267 | 18946.601563 | +0.527954 | 442.585937 | 217 |
| D5 | -7800 | -17267 | 18947.011719 | +0.525200 | 442.576324 | 217 |
| E1 | -164 | -351 | 387.423553 | -0.547844 | 2113.219727 | 225 |
| E2 | -164 | -351 | 387.423553 | -0.547844 | 2113.219727 | 213 |
| E3 | -164 | -351 | 387.423553 | -0.547844 | 2113.219727 | 213 |
| E4 | -163 | -352 | 387.908508 | -0.351501 | 2110.577637 | 217 |
| E5 | -163 | -352 | 387.908508 | -0.351501 | 2110.577637 | 217 |
| F1 | -163 | -351 | 387.001282 | -0.117630 | 2218.402344 | 217 |
| F2 | -163 | -352 | 387.908508 | -0.055420 | 2213.214111 | 213 |
| F3 | -163 | -351 | 387.001282 | -0.117630 | 2218.402344 | 218 |
| F4 | -164 | -352 | 388.329742 | -0.189308 | 2210.813232 | 213 |
| F5 | -163 | -352 | 387.908508 | -0.055420 | 2213.214111 | 218 |
| G1 | -1509 | -3342 | 3666.885010 | +0.220955 | 2248.275879 | 230 |
| G2 | -1510 | -3342 | 3667.296631 | +0.206718 | 2248.023682 | 213 |
| G3 | -1510 | -3342 | 3667.296631 | +0.206718 | 2248.023682 | 217 |
| G4 | -1510 | -3342 | 3667.296631 | +0.206718 | 2248.023682 | 217 |
| G5 | -1509 | -3342 | 3666.885010 | +0.220955 | 2248.275879 | 213 |
| H1 | -1509 | -3342 | 3666.885010 | +0.534843 | 2286.818115 | 217 |
| H2 | -1509 | -3341 | 3665.973633 | +0.528412 | 2287.386719 | 217 |
| H3 | -1509 | -3342 | 3666.885010 | +0.534843 | 2286.818115 | 213 |
| H4 | -1509 | -3342 | 3666.885010 | +0.534843 | 2286.818115 | 213 |
| H5 | -1509 | -3341 | 3665.973633 | +0.528412 | 2287.386719 | 213 |

## Resumen y conclusión

| Serie | Impedancia media (Ω) | Desviación estándar muestral (Ω) | Diferencia frente a nominal externo + 100 Ω* |
| --- | ---: | ---: | ---: |
| A | 306.459 | 0.059 | −4.23 % |
| B | 321.285 | 0.049 | +0.40 % |
| C | 435.105 | 0.015 | +35.97 % |
| D | 442.578 | 0.026 | +38.31 % |
| E | 2112.163 | 1.447 | −8.17 % |
| F | 2214.809 | 3.423 | −3.70 % |
| G | 2248.125 | 0.138 | −2.26 % |
| H | 2287.046 | 0.311 | −0.56 % |

*Comparación orientativa con **320 Ω** para A–D y **2300 Ω** para E–H, suponiendo que R2 interviene como una suma resistiva ideal en ambas rutas; no es una corrección aplicada a las medidas.*

Las cinco lecturas de cada serie son repetibles: la dispersión relativa máxima es **0.155 %** (F). Considerar R2 explica parte de la diferencia frente a la resistencia externa nominal: B queda a **+0.40 %** de 320 Ω y H a **−0.56 %** de 2300 Ω bajo el modelo simple. Sin embargo, las diferencias entre rutas y calibraciones son mucho mayores que la dispersión dentro de una serie: con la misma carga externa de 220 Ω nominales, las medias van de **306.459 Ω (A)** a **442.578 Ω (D)**; con 2,2 kΩ nominales, de **2112.163 Ω (E)** a **2287.046 Ω (H)**. Cambiar solo la calibración de E a F desplaza la media **102.646 Ω**. R2 no explica por sí sola estos cambios entre series. Los datos demuestran repetibilidad de la adquisición, pero todavía no validan la exactitud absoluta de la resistencia externa.

## Estado de la matriz

Las **40 adquisiciones** previstas de la matriz A–H están registradas: cinco por cada combinación de resistencia nominal, ruta EC y calibración.

La impedancia indicada es la del circuito aparente (`1 / (GF × magnitud DFT)`); **no se ha restado R2** ni aplicado constante de celda, cálculo de conductividad o compensación térmica. La fase corregida solo compensa el instrumento. Para estimar la resistencia externa a partir de la impedancia aparente habrá que medir el patrón real y verificar cómo interviene R2 en la transferencia eléctrica de ambas rutas antes de aplicar una sustracción de 100 Ω o ajustar la conversión.
````

### Registro original: hito2e2.md

````text
# Hitos 2E.2 y 2E.3 — Sonda EC real y conductividad estimada

## Alcance y configuración

La sonda EC de constante de celda nominal **K = 1 cm⁻¹** se conecta a J1 Yx-1/Yx-2 con el equipo apagado. El firmware registra impedancia aparente y calcula una **conductividad estimada, no calibrada**, mediante `1e6 × K / |Z aparente|` en µS/cm. El modo activo es RAW: no resta R2 de 100 Ω. La fase instrumental y la naturaleza compleja de la impedancia impiden tratar esta cifra como una conductividad absoluta validada. El hito 2E.4 queda reservado para soluciones patrón y validación metrológica.

Parámetros iniciales: **2000 Hz**, MCLK externo **1 MHz**, Range 1 (**2 Vpp nominales**), PGA ×1 y settling de 10 ciclos ×1. La salida nominal del AD5934 **no garantiza** esa tensión en los electrodos: el acoplamiento, los amplificadores y la impedancia de la sonda alteran la señal real. No hay detección fiable de saturación analógica, autorange ni barrido.

| Ruta | Máscara | RFB | Calibraciones compatibles |
| --- | --- | ---: | --- |
| HIGH | `0x81` | 100 Ω | `CAL_HIGH_100R`, `CAL_HIGH_1K` |
| LOW | `0x82` | 1000 Ω | `CAL_LOW_1K`, `CAL_LOW_10K` |

Seleccionar ruta y calibración de forma explícita y comprobar `cn0349 ec config` y `cn0349 conductivity config` antes de cada serie. `cn0349 conductivity start` reutiliza la adquisición EC no bloqueante; `cn0349 conductivity status` y `cn0349 conductivity result` permiten revisar estado, configuración congelada, DFT y estimación. `cn0349 conductivity stop` cancela una adquisición activa. `cn0349 ec start` sigue disponible cuando solo interesa la impedancia. Si `start` rechaza una combinación o la muestra falla, corregir la configuración o la causa del error antes de repetirla; no contar un resultado anterior como una adquisición nueva.

## Caracterización cualitativa 2E.2/2E.3

Estas muestras **no son patrones de conductividad** y no tienen un valor asignado en µS/cm. Usar recipientes limpios y seguros; no utilizar líquidos peligrosos ni incompatibles con la sonda. Comenzar por las muestras menos salinas y añadir la sal en pequeñas cantidades.

| ID | Muestra | Preparación cualitativa |
| --- | --- | --- |
| A | Agua de baja mineralización | Sin adición de sal |
| B | Agua del grifo | Sin adición de sal |
| C | Agua con sal | Anotar origen del agua y cantidad de sal añadida; datos de esta sesión pendientes |
| D (opcional, 2E.4) | Agua del grifo con más sal | Aumentar la cantidad respecto de C, sin concentración excesiva |

Para cada muestra:

1. Enjuagar la sonda con agua de baja mineralización; evitar arrastrar líquido de la muestra anterior.
2. Sumergir los electrodos a la misma profundidad, sin burbujas y sin tocar paredes o fondo. Mantener el mismo recipiente o una geometría equivalente.
3. Medir la temperatura de la muestra con el multímetro y esperar a que la lectura se estabilice. Anotar también la duración aproximada de esta espera.
4. Comprobar con `cn0349 ec config` la ruta, RFB y calibración elegidos. Ejecutar cinco veces `cn0349 conductivity start`, esperando el fin de cada adquisición y consultando `cn0349 conductivity result`. Registrar solo resultados nuevos `COMPLETE / VALID / NONE` con standby verificado; conservar aparte el diagnóstico de cualquier fallo mientras se resuelve.
5. Registrar la estimación en µS/cm y mS/cm junto al DFT y Z aparente. Si la señal DFT es muy pequeña, el estado informa un error o hay sospecha de saturación, detener la serie y seleccionar **manualmente** otra combinación compatible. Registrar el cambio como otra serie; no asumir que HIGH/LOW designan directamente una clase de conductividad.
6. Cuando sea eléctricamente adecuado, repetir cinco adquisiciones con la otra ruta o calibración compatible. Mantener temperatura, inmersión y tiempo de espera comparables. Anotar si aparece deriva entre la primera y quinta lectura.

## Registro de resultados

Completar una fila por serie de cinco adquisiciones válidas. Conservar por cada lectura REAL, IMAG, magnitud DFT, fase DFT, fase corregida, impedancia aparente, conductividad estimada, estado, error y tiempo; la ruta, máscara, RFB, calibración, ID, GF, frecuencia efectiva, excitación, PGA, modelo RAW, K nominal, R2 nominal, temperatura externa, fecha o identificador de sesión y condiciones de inmersión pertenecen al encabezado de esa serie. Los valores se copian de `conductivity result` y no se tratan como conductividad calibrada. El snapshot actual vive en RAM y conserva la configuración de la adquisición; estas tablas permiten archivar las lecturas para la futura WebApp BLE, sin implementar aún almacenamiento persistente.

| Sesión | Muestra / descripción | Temperatura multímetro (°C) | Ruta / máscara / RFB | Calibración / ID / GF | Frecuencia efectiva (Hz) | Observaciones |
| --- | --- | ---: | --- | --- | ---: | --- |
| Pendiente | A / agua de baja mineralización | — | — | — | — | — |
| Sesión grifo 1 | B / agua del grifo | Pendiente | LOW `0x82` / 1000 Ω | `CAL_LOW_1K` / ID 3 / GF 0.000000121290 | 1999.996582 | K nominal 1 cm⁻¹; RAW; R2 nominal 100 Ω; inmersión y temperatura sin documentar |
| Sesión sal 1 | C / agua del grifo con sal; cantidad sin documentar | Pendiente | LOW `0x82` / 1000 Ω | `CAL_LOW_1K` / ID 3 / GF 0.000000121290 | 1999.996582 | K nominal 1 cm⁻¹; RAW; R2 nominal 100 Ω; inmersión y temperatura sin documentar |
| Pendiente | D / agua del grifo con más sal | — | — | — | — | — |

| Muestra / serie | Lectura | REAL | IMAG | Magnitud DFT | Fase DFT (°) | Fase corregida (°) | Z aparente (Ω) | EC estimada (µS/cm) | Estado / error | Tiempo (ms) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| B / grifo 1 | 1 | -2131 | -3566 | 4154.216797 | -120.862045 | -6.322502 | 1984.658813 | 503.864929 | COMPLETE / NONE | 229 |
| C / sal 1 | 1 | -13563 | -14768 | 20051.154297 | -132.564514 | -18.024971 | 411.183441 | 2432.004639 | COMPLETE / NONE | 223 |

Las primeras adquisiciones B1 y C1 fueron `VALID`, sin error de hardware y con standby verificado. Con la misma ruta y calibración, la muestra salada muestra menor Z aparente (**411.183441 Ω** frente a **1984.658813 Ω**) y mayor conductividad RAW estimada (**2432.004639** frente a **503.864929 µS/cm**). La fase corregida también cambia de **−6.322502°** a **−18.024971°**; no puede atribuirse exclusivamente a los electrodos. No constan temperatura, cantidad de sal ni repeticiones suficientes para evaluar repetibilidad o exactitud.

Al cerrar cada serie, calcular media y desviación estándar muestral de Z, coeficiente de variación (`100 × desviación/media`), mínimo, máximo y variación de fase corregida; registrar las condiciones de estabilidad. Comparar si Z tiende a disminuir al aumentar la sal, la dispersión y deriva de cada muestra, las diferencias entre HIGH y LOW y el efecto de elegir otro GF con la misma ruta. Anotar combinaciones que produzcan señal DFT pequeña, error o posible saturación, sin inventar un umbral de saturación no validado. Una fase cambiante puede incluir polarización de electrodos, deriva térmica y efectos de la electrónica; la corrección instrumental de fase aún no valida la fase física de la celda.

### Comparación de rutas y repetibilidad

Usar solo combinaciones con señal utilizable: HIGH + `CAL_HIGH_100R`, HIGH + `CAL_HIGH_1K`, LOW + `CAL_LOW_1K` y LOW + `CAL_LOW_10K`. No asumir que mayor magnitud DFT significa mayor exactitud.

| Muestra / sesión | Ruta y calibración | n | Media Z (Ω) | s muestral (Ω) | CV (%) | Z mín.–máx. (Ω) | Fase mín.–máx. (°) | Observaciones |
| --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| Pendiente | — | — | — | — | — | — | — | — |

### Estabilidad temporal (reservado para 2E.4)

Con la sonda inmóvil en una muestra, hacer **diez adquisiciones espaciadas manualmente**. Registrar el tiempo relativo de cada una y mantener ruta, calibración, profundidad y temperatura lo más constantes posible. La secuencia sirve para detectar deriva; no requiere un temporizador nuevo en el firmware.

| Muestra / sesión | Ruta y calibración | Lectura 1–10 | Tiempo relativo (s) | Temperatura (°C) | Magnitud DFT | Z aparente (Ω) | Fase corregida (°) | Observación |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Pendiente | — | — | — | — | — | — | — | — |

## Soluciones patrón (reservadas para 2E.4)

En el futuro 2E.4, si se dispone de soluciones comerciales, preferir **84 µS/cm**, **1413 µS/cm** y **12,88 mS/cm**. Registrar fabricante, identificación, conductividad nominal, temperatura de referencia, tolerancia declarada y vigencia. Medir la temperatura real con el multímetro y repetir cinco adquisiciones con inmersión, espera y configuración documentadas. No comparar directamente una conductividad certificada a 25 °C con una medida a otra temperatura sin considerar la dependencia térmica del patrón. No ajustar K ni los GF con las lecturas actuales.

| Patrón / lote / sesión | Nominal y unidad | T ref. (°C) | T real (°C) | Tolerancia | Ruta / calibración | n | Media Z (Ω) | s (Ω) | Fase media (°) | Observaciones |
| --- | --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | --- |
| Pendiente | — | — | — | — | — | — | — | — | — | — |

Comparar la **tendencia eléctrica** y la dispersión entre rutas y calibraciones; la conductividad estimada por RAW no equivale a una conductividad calibrada ni permite deducir una constante de celda efectiva o una corrección automática a partir de este hito. Dejar explícito cuando no se disponga de patrones; 2E.4 no se declara ejecutado por preparar este protocolo.

## Limitaciones observadas

| Muestra / configuración | Señal DFT pequeña, error o sospecha de saturación | Deriva / fase / temperatura | Acción y resultado |
| --- | --- | --- | --- |
| Pendiente | — | — | — |

## Estado de validación

El firmware reutiliza los comandos EC existentes y conserva los snapshots por adquisición. Las pruebas host verifican compatibilidad de calibración, errores, standby, configuración conservada, cálculo RAW y regresión de Pt100/temperatura; **no simulan** la sonda ni el líquido. La compilación del ESP32-S3 finaliza correctamente; las primeras adquisiciones físicas con la sonda real siguen pendientes. La exactitud absoluta, la constante de celda efectiva, la sustracción de R2 y la compensación térmica quedan pendientes de validación metrológica en 2E.4.
````
