# WebApp BLE de pruebas — Hito 3A

## Interfaz BLE

La web adopta el diseño oscuro del proyecto Spectrometer TCS3448: cabecera de
conexión, navegación lateral, tarjetas y vistas adaptables a móvil. Measurement
muestra temperatura PT100 y conductividad EC; Setup mantiene los parámetros,
la selección de sensores y APPLY/CANCEL; Calibration conserva CAL RUN y la
tabla de referencias. La lectura y las órdenes siguen usando el protocolo BLE
actual de CN0349; no se añade un gráfico espectral ni funciones de datos que
este dispositivo no ofrece.

MEASUREMENT recibe el último ciclo por notificación. Al confirmar STOP, se
vuelve a leer MEASUREMENT por BLE para mostrar el último ciclo disponible. Los
snapshots CANCELLED de la parada no reemplazan los últimos datos mostrados; las
notificaciones continúan actualizando cada ciclo no cancelado. Los botones se
habilitan según STATUS/busy; continuo puede estar en READY entre ciclos pero
mantiene STOP habilitado por `busy=true`.

El canal CONTROL usa paquetes de cuatro bytes y confirmación correlacionada por
requestId. No se duplican parsers del Terminal ni cálculos. La página conserva
file://, Web Bluetooth nativo, desconexión explícita y ausencia de reconexión
automática, frameworks, servidor, Wi-Fi, almacenamiento y gráficas.

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
órdenes BLE, almacenamiento, gráficas ni reconexión automática.
