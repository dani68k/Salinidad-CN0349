# WebApp BLE de pruebas — Hito 3A

## Interfaz BLE

La web adopta el diseño oscuro del proyecto Spectrometer TCS3448: cabecera de
conexión, navegación lateral, tarjetas y vistas adaptables a móvil. Measurement
muestra temperatura PT100 y conductividad EC; Setup mantiene los parámetros,
la selección de sensores y APPLY/CANCEL; Calibration conserva CAL RUN y la
tabla de referencias. La lectura y las órdenes siguen usando el protocolo BLE
actual de CN0349; no se añade un gráfico espectral ni funciones de datos que
este dispositivo no ofrece.

Setup permite elegir si la conductividad EC se muestra en µS/cm, mS/cm o ambas
unidades. Es una preferencia visual local a la página: se aplica inmediatamente
y no modifica ni envía la configuración del instrumento. Por defecto se muestra
en µS/cm.

En Measurement, la temperatura se muestra como “Temperature with PT100” y la
conductividad como “Conductivity with” seguida del modelo EC de la última
medida (o de la configuración confirmada antes de recibir medidas). Cada
tarjeta muestra únicamente su valor, con unidades en el propio valor cuando
corresponde; el modelo no se repite junto al valor EC.

Debajo de los medidores, Measurement dibuja el historial de los últimos 50
ciclos recibidos. Temperatura y conductividad usan escalas verticales
independientes; la escala EC usa mS/cm cuando se selecciona esa unidad y µS/cm
para las otras opciones. Los ciclos cancelados no se añaden al historial y los
valores fallidos dejan un hueco en la serie correspondiente.
El selector AUTO/FIXED aparece debajo de la gráfica, alineado a la izquierda.
AUTO ajusta los ejes a los valores recibidos; los límites de FIXED se configuran
en Setup para temperatura (°C) y EC (µS/cm). Los límites EC se convierten al
mostrar la gráfica en mS/cm.

MEASUREMENT recibe el último ciclo por notificación. Al confirmar STOP, se
vuelve a leer MEASUREMENT por BLE para mostrar el último ciclo disponible. Los
snapshots CANCELLED de la parada no reemplazan los últimos datos mostrados; las
notificaciones continúan actualizando cada ciclo no cancelado. Los botones se
habilitan según STATUS/busy; continuo puede estar en READY entre ciclos pero
mantiene STOP habilitado por `busy=true`.

El canal CONTROL usa paquetes de cuatro bytes y confirmación correlacionada por
requestId. No se duplican parsers del Terminal ni cálculos. La página conserva
file://, Web Bluetooth nativo, desconexión explícita y ausencia de reconexión
automática, frameworks, servidor, Wi-Fi ni almacenamiento local.

## Hito 3B

Setup edita sensores, intervalo, rango, K y modelo. Al conectar se lee SETUP y
se suscribe a SETUP/SETUP_RESULT. APPLY valida, envía un paquete de 13 bytes,
espera el resultado con ID y vuelve a leer la configuración aplicada; CANCEL
restaura la última confirmada. Una notificación SETUP del terminal actualiza el
formulario. Edición y APPLY permanecen deshabilitados fuera de READY, durante
APPLY o sin conexión; volver a READY nunca aplica cambios automáticamente.

Abrir `index.html` directamente. La página usa HTML/CSS/JavaScript nativos y
Web Bluetooth para buscar `CN0349`, conectar al servicio ATLANTES, leer STATUS
y suscribirse a notificaciones. Se separan conexión, estado, calibraciones,
ocupación y errores. Una desconexión inesperada actualiza la interfaz; no hay
reconexión automática.

UUID de servicio: `8d7b0001-6f7a-4f31-9c4d-2b6e6a7f3a10`.
UUID STATUS: `8d7b0002-6f7a-4f31-9c4d-2b6e6a7f3a10`.

Web Bluetooth depende del navegador y del contexto seguro. Si `file://` no
expone `navigator.bluetooth`, la página lo indica; para la prueba manual puede
ser necesario servir `web/` mediante `localhost`. No hay dependencias externas,
almacenamiento local ni reconexión automática.

Setup muestra el perfil AD5934 configurado en un panel de solo lectura y lo
actualiza mediante ADVANCED; no representa una lectura actual de registros. Si
la caracteristica no esta disponible, muestra los valores como no disponibles.
La distribucion apila los paneles en pantallas pequenas.

Measurement usa EC_DIAGNOSTIC para colorear el valor EC de la misma medida:
blanco dentro de rango o con diagnóstico inválido, naranja cerca del límite
o por debajo, y rojo por encima. LOW por encima sugiere HIGH; HIGH por debajo
sugiere probar LOW sin garantizar que la medida esté en su intervalo. El valor
y el diagnóstico se asocian por `cycleId`; si falta el diagnóstico, se muestra
el valor sin color de diagnóstico tras un segundo.
