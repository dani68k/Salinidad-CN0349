# Driver AD5934

Acceso I²C al AD5934 mediante `AD5934(TwoWire&, address)`; la dirección por
defecto es 0x0D. El driver no inicializa Wire ni interpreta valores como
impedancia o conductividad. ImpedanceApp coordina la secuencia.

## Interfaz

- `begin()` sondea la dirección sin escribir registros.
- `readRegister8/16/24(reg, value)` y `writeRegister8/16/24(reg, value)`
  validan dirección y grupo de registros; devuelven `bool`.
- `readStatus(value)`, `readReal(value)` y `readImaginary(value)` leen
  estado o datos DFT con signo.
- `setStartFrequencyCode(code)`, `setFrequencyIncrementCode(code)`,
  `setNumberOfIncrements(count)`, `setSettlingCycles(cycles)` y
  `configureSweep(startCode, settlingWord)` programan campos válidos.
- `configureRange1PgaX1()`, `wakeToStandby(previous, verified)`,
  `enterStandby(previous, verified)` y `sendCommand(command)` gestionan
  control y transiciones permitidas.
- `frequencyToCode(frequencyHz, mclkHz, code)` valida y calcula la palabra
  DDS; `magnitude(real, imaginary)` calcula magnitud DFT.
- `lastError()` devuelve código, fase, registro, código Wire y bytes
  completados. `validateRead/Write` permiten validar un acceso.

La aplicación usa MCLK externo 1 MHz, frecuencia 2 kHz, Range 1 y PGA ×1.
El DDS calcula `floor(frequency * 2^27 / MCLK)`; la frecuencia efectiva
está cuantizada. El ACK I²C no demuestra la presencia física del reloj.

Las lecturas usan el puntero de dirección y las escrituras multibyte se realizan
byte a byte; un fallo puede dejar una escritura parcial. El driver valida
registros y control, conserva los bits relevantes y verifica transiciones.
No valida amplitud, continuidad analógica, exactitud ni saturación. Las
transacciones son síncronas y dependen del timeout configurado en Wire.

Referencia: [datasheet AD5934](https://www.analog.com/media/en/technical-documentation/data-sheets/ad5934.pdf).

Las interfaces y tipos públicos de `AD5934.h` forman parte de la referencia
HTML generada con `doxygen Doxyfile` desde la raíz del proyecto.
