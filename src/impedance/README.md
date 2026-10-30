# ImpedanceApp

Coordina AD5934 y MuxApp para una adquisición DFT no bloqueante. No interpreta
comandos ni convierte a temperatura o conductividad.

- `configureSingle(preset, frequencyHz, settlingCycles)` valida y configura el
  perfil de referencia mientras no está ocupado; devuelve `false` si falla.
- `requestSingle(busReady)`, `requestPt100(busReady)` y
  `requestEc(busReady, mask)` solicitan la adquisición para la ruta indicada;
  devuelven `false` si no puede iniciarse.
- `update()` avanza una etapa. `cancelOperation()` solicita parada segura.
- `singleResult(out)` copia REAL, IMAG, magnitud, fase y frecuencia solo en
  Complete. `statusSnapshot()`, `measurementState()`, `profile()` e
  `isBusy()` consultan estado; `requestRecovery(busReady)` intenta recuperar
  y comprobar hardware.

El perfil operativo usa MCLK externo 1 MHz, 2 kHz, Range 1/PGA ×1, incremento
cero y 10 ciclos. Se configuran y verifican registros antes de Initialize y
Start; D1 se consulta cada 5 ms. El timeout DFT es 500 ms y el de las fases de
configuración/control 1500 ms. La parada verifica standby; si falla, se
informa y no se declara hardware listo. Las transacciones I²C son síncronas,
acotadas por el timeout de Wire.

La API y los tipos públicos están documentados en `ImpedanceApp.h` para la
referencia Doxygen generada con `doxygen Doxyfile` desde la raíz del proyecto.
