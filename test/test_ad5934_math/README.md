# Pruebas matemáticas AD5934

Suite Unity host para funciones deterministas de `include/ad5934_math.h` usadas
por el driver AD5934: palabra DDS, rango respecto al MCLK, decodificación signed
16-bit, magnitud de ancho ampliado, bits reservados de settling y transiciones
seguras del registro de control.

Ejecutar con `pio test -e native` en un equipo con PlatformIO Native y compilador
C/C++ disponible. No accede a Wire ni simula registros o respuestas de hardware;
timeouts, secuencia I²C y recuperación física se verifican con el plan de
[`test/hito1d1.md`](../hito1d1.md).

Hito 2D, 2026-10-27: también ejecutable con
`test\host_cn0349\run_math.cmd` (MSVC/Unity, 11 pruebas PASS). Se corrigió la
referencia truncada 46340.950 a 46340.95001184158 = sqrt(2)*32768 en la prueba
de magnitud; el truncamiento excedía la tolerancia 1e-5. No cambia el algoritmo.
El entorno native activa `UNITY_INCLUDE_DOUBLE` para ejecutar estas aserciones.
