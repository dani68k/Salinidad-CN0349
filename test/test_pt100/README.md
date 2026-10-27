# Pruebas puras IEC 60751

`test_main.cpp` usa Unity y solo incluye el conversor Pt100; no necesita Arduino
ni dobles de hardware. Una ecuación directa de referencia con constantes
completas genera las resistencias de -200, -100, 0, 20, 25, 40, 100 y 850 °C.
Tres pruebas agrupan inversión, continuidad alrededor de cero y errores
(límites estrictos, cero, negativos, NaN e infinitos). Se comprueban validez,
código y ausencia de temperatura en error. La tolerancia de puntos es 1e-8 °C.

Ejecutar `pio test -e native -f test_pt100` con gcc/g++ disponible.
`platformio.ini` activa `UNITY_INCLUDE_DOUBLE` para evitar aserciones double
deshabilitadas. En este equipo ejecutar `test\host_cn0349\run_math.cmd`:
compila esta misma suite y la suite AD5934 con MSVC y Unity ya instalado en
`.pio/libdeps/native/Unity`. Los binarios quedan en `.pio/host_math`.

2026-10-27: MSVC, 3 pruebas PASS. PlatformIO native no ejecutado con éxito:
faltan gcc/g++. No valida hardware, sensor, coeficientes medidos ni exactitud absoluta.
