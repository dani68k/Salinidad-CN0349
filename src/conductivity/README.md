# Conductivity

`Conductivity.h` contiene conversiones puras sin Arduino, I²C ni acceso a
hardware. `Result` conserva modelo, valores, validez y código `Error`.

## Modelos e interfaces

- `convert(apparentImpedanceOhms, nominalKPerCm, model, r2Ohms)` recibe
  impedancia aparente en Ω, K en cm⁻¹ y R2 en Ω. Para RAW usa la impedancia
  como resistencia estimada; SERIES usa impedancia menos R2. Devuelve EC en
  µS/cm y mS/cm, o un error si las entradas o resultados no son válidos.
- `calibrateAnalogDevices(nLow, nHigh, yLowSiemens, yHighSiemens, rfbOhms,
  frequencyHz, excitationRange, pgaGain, calibrationId)` calcula GF y NOS a
  partir de dos referencias y devuelve `AdCalibration` con validez/error.
- `convertAnalogDevices(magnitudeCode, nominalKPerCm, r2Ohms, calibration,
  measurementFrequencyHz, expectedRfbOhms, excitationRange, pgaGain)` usa la
  magnitud DFT y la calibración AD, comprueba compatibilidad y devuelve EC y
  los valores intermedios de admitancia.

RAW, SERIES y AD se pueden seleccionar desde el controlador. SERIES es un
modelo experimental: la transferencia real y el efecto de R2 requieren
caracterización. K y R2 iniciales son nominales. La impedancia aparente no
demuestra exactitud de una celda real. EC25 se calcula después, en el
controlador, y no modifica estas fórmulas.

Las funciones y tipos públicos de `Conductivity.h` forman parte de la
referencia HTML generada con `doxygen Doxyfile` desde la raíz del proyecto.
