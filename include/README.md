# Cabeceras compartidas

- `config.h`: nombre e identificador de firmware, pines, velocidades,
  direcciones I²C, timeout, valores EC nominales y límites del terminal.
  Actualmente declara versión 1.0.0 e identificador 3B. No contiene el Setup
  mutable; este reside en `CN0349Manager`.
- `ad5934_math.h`: funciones puras de conversión DDS, frecuencia efectiva,
  decodificación y magnitud/fase DFT, validación de barrido y control.
- `impedance_diagnostics.h`: códigos y registros de error, subsistema,
  recuperación y contadores utilizados por ImpedanceApp.

`main.cpp` inicializa Wire y aplica los valores de `config.h`. Si se cambia
`SERIAL_BAUD`, debe mantenerse coherente con `monitor_speed` en
`platformio.ini`. Estas cabeceras no realizan I²C ni imprimen.

Sus constantes, tipos y funciones están documentados en la referencia HTML
generada con `doxygen Doxyfile` desde la raíz del proyecto.
