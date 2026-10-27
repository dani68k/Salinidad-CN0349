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
