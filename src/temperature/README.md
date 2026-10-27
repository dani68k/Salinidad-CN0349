# Conversión Pt100 — Hito 2D, 2026-10-27

`Pt100.h` es un módulo puro, sin Arduino, I2C ni drivers. La interfaz
`Pt100::convert(double ohms)` devuelve `Result {valid, celsius, error}`.
En error la temperatura es NaN; `NotAttempted` representa conversión no ejecutada.
`resistance(t)` evalúa el modelo directo para temperaturas dentro del intervalo
nominal; la validación de entrada pertenece a `convert`.

IEC 60751 / Callendar–Van Dusen, con R0=100 ohm, A=3.9083e-3,
B=-5.775e-7 y C=-4.183e-12:

```text
T >= 0: R = R0 (1 + A T + B T²)
T <  0: R = R0 [1 + A T + B T² + C (T-100) T³]
```

La inversión positiva usa `d=R/R0-1` y la raíz racionalizada
`T=2d/(A+sqrt(A²+4Bd))`, estable cerca de cero. La negativa usa 48 pasos
de bisección sobre [-200,0], con error de intervalo inferior a 1e-12 °C.
Se rechazan valores no finitos, no positivos y resistencias fuera de
[R(-200),R(850)] (aproximadamente [18.52008,390.481125] ohm).
No se amplían los límites para aceptar valores redondeados de una tabla.

La exactitud numérica no valida la exactitud de la adquisición. Se convierte
Z aparente a temperatura experimental suponiendo comportamiento resistivo.
No hay offsets, autorange ni compensación de cables. El módulo no conoce la
calibración: la asociación pertenece al snapshot del Manager. Una compensación
futura deberá ser una política explícita entre adquisición y conversión,
conservando siempre Z original; hoy `cableCompensationApplied` es siempre false.

Pruebas puras: [test_pt100](../../test/test_pt100/README.md).
Procedimiento físico: [hito2d](../../test/hito2d.md).
