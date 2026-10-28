# Suite host CN0349 — Hito 2F (2026-10-27)

## Hito 2G

Un octavo grupo cubre extremos AD, caso sintético, entradas no finitas, spans
inválidos, denominador cero, incompatibilidad, invalidez por rango, snapshots,
Pt100+EC, once/continuo y terminal. RAW, SERIES y Pt100 siguen en regresión.

Ejecutar `test\host_cn0349\run.cmd` desde la raíz en Windows. Usa MSVC actual o
Build Tools 2019 en su ubicación local. Compila producción real: Manager,
ImpedanceApp, MuxApp, Terminal y ambos drivers. Binario en `.pio/host_cn0349`.
Solo Arduino/Stream/millis y Wire se sustituyen por dobles.

Wire simula registros, DFT, rutas y fallos: dispositivo ausente, error de bus,
timeout DFT por máscara, DFT cero por máscara, readback de mux y fallo de standby.
El reloj se avanza explícitamente. No hay acceso a placa ni puerto serie.
Stream comprueba texto, pero no reproduce el redondeo decimal de Arduino.

Siete grupos PASS: matemáticas, arranque, calibración parcial/manual,
selección/ciclos, continuo/tiempo, stop/errores y terminal. Se comprueban las
tres selecciones, secuencia de rutas, snapshots, incompatibilidad de perfil,
stop temprano y durante ambos sensores, millis wrap, 1000 ciclos, sobrescritura
de resultados, hardware ausente, timeout/I²C/readback/standby y recuperación.
También selección inválida/duplicada, busy, comandos retirados, CRLF y overflow.

`run_math.cmd` ejecuta sin modificaciones las fuentes Unity de Pt100 (3 PASS) y
AD5934 (11 PASS). Requiere Unity en `.pio/libdeps/native/Unity`; binarios en
`.pio/host_math`. MSVC muestra C4018 preexistente en AD5934.cpp; los drivers
no se modifican. No se afirma una ejecución PlatformIO native: las suites se
ejecutan con MSVC, mientras PlatformIO compila el firmware ESP32-S3.

Las pruebas anteriores del gestor/comandos históricos se sustituyen por el
contrato 2F y se conservan las pruebas de fórmulas relevantes. Documentos de
hitos anteriores permanecen como historial. Cobertura y pruebas físicas
pendientes: [hito2f.md](../hito2f.md).
