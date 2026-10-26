# Aplicación ESP32-S3

`main.cpp` inicializa Serial y Wire, crea una instancia de los drivers AD5934 y
ADG715, y conecta ImpedanceApp y MuxApp al terminal. El ciclo llama a
`Terminal::update()` y a las actualizaciones de aplicación sin esperas de entrada
ni `delay()`.

`terminal/`, `impedance/` y `mux/` están implementados en Hitos 1A–1D.2.
ImpedanceApp coordina adquisiciones DFT de punto único y barrido; MuxApp aplica
presets autorizados y verifica el interlock. `cn0349/` documenta el Manager
futuro. No hay cálculo de impedancia, calibración, conductividad, Pt100 ni BLE.
