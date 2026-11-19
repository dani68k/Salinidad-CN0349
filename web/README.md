# WebApp BLE

Interfaz HTML/CSS/JavaScript nativa para el instrumento CN0349. Abrir
`index.html` en un navegador compatible con Web Bluetooth; si `file://` no
ofrece la API, servir la carpeta desde `localhost`. Usa el servicio y las
características descritos en [BLE](../src/ble/README.md). No requiere framework
ni servidor propio y no reconecta automáticamente.

## Measurement

Muestra la última temperatura Pt100, EC medida y modelo. Si la compensación
está activa, muestra EC25 o un indicador de indisponibilidad. Bajo la
temperatura Pt100 muestra la resistencia aparente recibida de la adquisición
del mismo ciclo. El color de EC procede del diagnóstico del firmware: blanco
dentro de rango o inválido,
naranja cerca del máximo o por debajo y rojo por encima. LOW por encima sugiere
HIGH; HIGH por debajo sugiere probar LOW sin garantizar que esté en rango. No
se cambia el rango automáticamente.

El historial conserva hasta 50 ciclos en memoria. El gráfico usa un eje de
temperatura y otro compartido por EC y EC25; los puntos inválidos dejan huecos.
EC25 se muestra cuando la compensación está activa. AUTO ajusta las escalas;
FIXED usa los límites configurados en Setup. CLEAR vacía el historial visible.
Los ciclos cancelados no reemplazan las últimas lecturas.

## Setup y Calibration

### Calibración de la sonda EC

Setup permite introducir `a`, `b` y `c` para LOW y HIGH mediante la regresión
polinómica de segundo grado de Excel. Utilizar como `x` las lecturas EC25 con
compensación de temperatura activa y calibración de sonda desactivada; usar
como `y` las conductividades de referencia en µS/cm. Con **Enable** activo,
el instrumento calcula `EC25 corregida = a * EC25² + b * EC25 + c`. Los tres
coeficientes deben ser finitos y se requieren ambos rangos para activar la
corrección. Enable desactivado conserva los coeficientes sin aplicarlos.

Las calibraciones lineales anteriores se cargan con `a = 0`, `b = m` y
`c = b` de la recta original.

### Calibración lineal anterior

La versión lineal permitía introducir los coeficientes `m` y `b` obtenidos,
por ejemplo, mediante regresión en Excel, por separado para LOW y HIGH. El
ajuste se realiza con las lecturas EC25 sin corrección de sonda como `x` y
los valores de referencia como `y`, ambos en µS/cm. Con **Enable** activo,
el instrumento aplicaba `EC25 corregida = m * EC25 + b`; con Enable inactivo
conservaba los coeficientes sin aplicarlos. Para activar la corrección se
requerían ambos rangos, con `m` finita positiva y `b` finita.

Esta descripción corresponde a la versión lineal anterior.

Setup edita intervalo, rango, modelo, K, sensores y compensación. APPLY envía
los cambios conjuntamente y espera confirmación; CANCEL restaura el último
Setup confirmado. ON requiere Pt100 + EC; una selección incompatible muestra
un aviso y bloquea APPLY. Alpha admite 0–5 %/°C y la referencia es 25 °C.

Advanced Setup presenta el perfil AD5934 configurado como READ ONLY; no muestra
una lectura actual de registros ni permite editarlo. Las unidades EC y los
límites de la gráfica son preferencias de la página y no modifican el
instrumento. Debajo del perfil, la calibración lineal Pt100 permite ingresar
temperatura patrón y resistencia medida para dos puntos A/B, en orden
ascendente (A inferior, B superior) para mantener m positiva; muestra la
pendiente y el intercepto calculados, y guarda la selección y los puntos en el
instrumento. ON aplica `T=m·R+b` en el firmware, incluida EC25; OFF conserva
IEC 60751. La recta extrapola fuera del intervalo A-B. Calibration muestra las
referencias y permite solicitar CAL RUN.

Los valores EC, EC25 y el diagnóstico se asocian por `cycleId`; la WebApp no
recalcula EC25 ni el diagnóstico. Si no llega el diagnóstico, la EC se muestra
sin color de diagnóstico tras un segundo. Al desconectarse se limpian lecturas
e historial visibles.
