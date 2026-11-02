# Conversión Pt100

`Pt100.h` implementa IEC 60751 / Callendar–Van Dusen sin Arduino ni I²C.

- `Pt100::convert(ohms)` recibe resistencia en Ω y devuelve `Result` con
  validez, temperatura en °C y error. Una entrada no finita, no positiva o
  fuera del intervalo nominal produce un resultado inválido con temperatura NaN.
- `Pt100::resistance(celsius)` evalúa el modelo directo dentro de su intervalo
  nominal; la validación de entrada pertenece a `convert()`.
- `Pt100::convertLinear(ohms, temperatureA, resistanceA, temperatureB,
  resistanceB)` calcula `m=(T2-T1)/(R2-R1)`, `b=T1-m*R1` y `T=m*R+b`.
  Exige que A sea inferior a B en temperatura y resistencia, para que m sea
  positiva; extrapola fuera de los puntos y devuelve error si el resultado no
  es representable.

Usa R0=100 Ω, A=3,9083×10⁻³, B=−5,775×10⁻⁷ y C=−4,183×10⁻¹².
La inversión positiva usa la raíz racionalizada y la negativa bisección en
[−200, 0] °C. El intervalo nominal es −200 a 850 °C.

La conversión IEC de la impedancia aparente supone comportamiento resistivo.
La recta de dos puntos puede compensar el conjunto sensor/cable, pero solo
representa una aproximación lineal y no demuestra exactitud física.
[Validación histórica](../../test/VALIDACION.md).

Las funciones, constantes y tipos públicos de `Pt100.h` forman parte de la
referencia HTML generada con `doxygen Doxyfile` desde la raíz del proyecto.
