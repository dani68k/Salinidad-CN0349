# Pruebas host Hito 2C.1

Ejecutar desde la raíz: `test\host_cn0349\run.cmd` (Windows, MSVC C++).
El script usa el entorno de Visual Studio actual si está inicializado; como
fallback llama a BuildTools 2019 instalado en este equipo. Para otro compilador,
compilar los mismos fuentes e incluir esta carpeta antes que Arduino/Wire.

Compila **código de producción real**: Manager, ImpedanceApp, MuxApp, Terminal y
ambos drivers. Solo Arduino/Stream/reloj y TwoWire se sustituyen por dobles de
prueba. No hay conexión a placa ni puerto serie. Los binarios van a `.pio/host_cn0349`.
`Wire.h` emula registros y DFT constante controlada; no emula el circuito analógico,
la temporización real del conversor ni todos los códigos I²C. Stream no verifica
el formato decimal ni hexadecimal de Arduino. `strtok_r` se adapta a `strtok_s`
solo en host MSVC. No añadir esta carpeta a los includes del firmware.

Cobertura ejecutada el 2026-10-27, tres grupos PASS:

- Matemática: fórmula inversa, normalización en ambos sentidos y límite 180°,
  cero/negativos/NaN/Inf, overflow/underflow y redondeo DDS de 2B.
- Manager/terminal/regresión: arranque, calibración ausente/desconocida/LOW,
  cuatro calibraciones 2B, ambas selecciones HIGH, medida con máscara 0x41,
  busy por adquisición y escáner, metadatos/DFT/ohmios, frecuencia y settling
  incompatibles, cancelación y reanudación, stop heredado, DFT cero y timeout,
  último válido conservado, single, sweep 9 puntos y diagnóstico, conservación
  de IDs/GF de los cuatro slots, comandos y consultas sin transacciones I²C.
- Fallos de readback ADG715 y de standby: FAILED y nunca resultado válido.

Los valores DFT sintéticos permiten verificar el cálculo sin resistencias
esperadas en producción. No validan las cargas físicas 220 Ω/2200 Ω ni precisión.
La compilación MSVC muestra un warning C4018 preexistente en el driver AD5934.
`pio test -e native` sigue sin ejecutarse por ausencia de gcc/g++; esta suite
MSVC ofrece una alternativa ejecutada, no un resultado PlatformIO native PASS.

## Hito 2D — 2026-10-27

`run.cmd` incluye CN0349Temperature.cpp: cuatro grupos PASS. Las pruebas nuevas
ejercitan calibración ausente, busy, adquisición de temperatura con 100 ohm,
HIGH_1K fuera de rango con DFT válido, cancelación, timeout, fallo de mux,
errores upstream inconsistentes, comandos sin I2C y aviso experimental. Guardan
ID/GF/temperatura y comprueban su conservación tras selección, recalibración y
adquisición de impedancia independiente. Wire sigue siendo un doble: no se
deduce exactitud física.

`run_math.cmd` ejecuta las mismas fuentes Unity de `test_pt100` (3 PASS) y
`test_ad5934_math` (11 PASS), con double activado. Requiere Unity ya presente en
`.pio/libdeps/native/Unity` y Build Tools 2019. `unity_hooks.c` aporta los hooks
vacíos que PlatformIO normalmente genera para la suite AD5934. La suite Pt100
declara sus propios hooks. Estas suites puras no enlazan dobles ni drivers.
Los binarios matemáticos se guardan en `.pio/host_math`.

## Preparación de arranque — 2026-10-27

`run.cmd`: cinco grupos PASS tras añadir `startupTests`. Comprueba recover
antes de calibrar, cuatro slots con cinco muestras a 2000 Hz, busy y rechazo
de una segunda preparación, ausencia de anuncio listo antes de terminar,
selección RTD conservada y temperatura válida posterior. Comprueba también
cancelación, timeout DFT y fallo de recuperación sin iniciar calibración,
así como la regresión del comando manual `cal run all`. El fallo de recovery
se simula partiendo de modo 0x21 e impidiendo escribir standby. Continúa sin
acceso a hardware físico; no verifica la temporización real al encender.

## Temperatura continua — 2026-10-27

`run.cmd`: seis grupos PASS. `continuousTemperatureTests` comprueba rechazo sin
calibración, primera adquisición inmediata, reserva busy, rechazo de cambio de
calibración y escáner I2C entre muestras, segundo inicio a 1000 ms y ausencia de
inicio a 999 ms atravesando el desbordamiento de millis. Comprueba salida
automática, stop durante adquisición, stop entre muestras conservando resultado,
ausencia de transacciones futuras tras stop, parada por timeout y stop genérico.
La temporización usa reloj simulado; queda pendiente comprobar la cadencia real
y salida serie sobre la placa.

Salida continua simplificada, 2026-10-27: los seis grupos continúan PASS.
La prueba comprueba el prefijo `temperature: ` y sufijo UTF-8 `ºC`, la ausencia
de mensajes RTD/DFT/REAL/standby y la conservación del detalle en `temperature
result`. También comprueba que la siguiente medida única recupera su informe
RAW. El doble Stream no reproduce el redondeo decimal Arduino: la implementación
de producción usa `print(celsius, 2)`; el formato visual queda pendiente en placa.

## Hito 2E.1 — 2026-10-27

`run.cmd` compila también `CN0349Ec.cpp`; siete grupos PASS en host. EC se
prueba con ambas máscaras y cuatro calibraciones, sin slot, RFB incompatible,
frecuencia/settling incompatibles, busy, readback ADG715 fallido, DFT cero,
timeout, cancelación, recuperación, standby y snapshot de resultado. Se
comprueba que la temperatura previa no cambia tras EC. `Wire.h` simula
registros y DFT fijo: no modela R2, señal analógica, saturación, exactitud ni
los 40 ensayos físicos. Ver [protocolo](../hito2e1.md).

## Hitos 2E.2 y 2E.3 — 2026-10-27

`run.cmd`: ocho grupos PASS. La matemática de `Conductivity::convert` comprueba
RAW con cinco resistencias de referencia, la fórmula serie experimental solo
como cálculo puro, entradas no finitas, K inválida, resistencia no positiva,
modelo inválido y desbordamiento. La integración prueba el rechazo del modelo
serie sin topología verificada, inicio asíncrono con ruta LOW, snapshot DFT y
calibración conservados, salida de terminal, rechazo de calibración incompatible,
resultado inválido ante DFT cero y cancelación. El fake no simula electroquímica
ni tensión en electrodos; no sustituye la prueba física con sonda real.
[Protocolo](../hito2e2.md).
