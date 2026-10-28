# Firmware — Hito 2F (2026-10-27)

Propósito: instrumento cooperativo Pt100/EC, preparado para reutilización BLE.
`main.cpp` crea un solo driver AD5934, ADG715, MuxApp, ImpedanceApp,
CN0349Manager y Terminal. Inicializa Wire y Serial, sin esperar conexión del
monitor, y llama `instrument.begin(busReady)` seguido de `terminal.begin()`.

El loop llama `instrument.update()` y `terminal.update()`. El primero progresa
calibración, adquisición y ciclos; el segundo interpreta comandos y consume
una vez cada resultado. No hay tareas, delays ni instancias paralelas de I²C.
Solo main/Terminal conocen Serial. Pines, velocidades y timeout están en
`include/config.h`. Los drivers locales no cambian en este hito.

Estados públicos: NotInitialized, Calibrating, Idle, Measuring, Stopping y Error.
Los errores de hardware impiden iniciar medidas; las calibraciones parciales
permiten las funciones cuyos slots son válidos. Ver [arquitectura y
API](../docs/architecture.md), [pruebas](../test/hito2f.md) y README de cada módulo.
