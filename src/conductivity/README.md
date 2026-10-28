# Módulo Conductivity — Hito 2E.3

## Hito 2G — ANALOG_DEVICES

`AdCalibration`, `calibrateAnalogDevices` y `convertAnalogDevices` son
funciones puras. Separan el Gain Factor legacy de la pendiente AD e implementan
las ecuaciones 1–4 de CN-0349 Rev. A con `double`. Devuelven ID, NX, GF, NOS,
YX, YCELL, denominador, K, unidades y error. K sigue nominal y no hay patrón
certificado, compensación térmica o validación física.

`Conductivity.h` es una conversión pura, sin I²C ni acceso a AD5934/ADG715.
Recibe impedancia aparente, K nominal, modelo y R2 nominal; devuelve
resistencia de celda estimada, µS/cm, mS/cm, validez y error explícito.

En RAW: `R_estimada = Z_aparente`. En el modelo matemático
SERIES_R2_EXPERIMENTAL: `R_estimada = Z_aparente − R2`. Después:
`µS/cm = 10⁶ × K / R_estimada` y `mS/cm = (µS/cm) / 1000`. Se comprueban
entradas finitas, K y resistencia positivas y resultados representables. El
modelo serie puede probarse matemáticamente, pero **no está habilitado en el
Manager** porque la equivalencia eléctrica de R2 en la placa concreta no está
verificada. Los valores configurados están en `include/config.h`:
K nominal = 1 cm⁻¹ y R2 nominal = 100 Ω.

La conversión toma la magnitud de impedancia como aproximación resistiva. Una
celda real puede tener fase y `|Z| ≠ Re(Z)`; la estimación no equivale a una
conductividad calibrada ni está compensada a 25 °C. El Manager conserva en su
snapshot el resultado EC original y el modelo aplicado; cambiar después ruta,
calibración o modelo no reinterpreta adquisiciones anteriores. La futura BLE
podrá transportar el snapshot, pero no forma parte de este módulo.
