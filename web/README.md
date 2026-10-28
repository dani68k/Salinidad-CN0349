# WebApp BLE de pruebas — Hito 3A

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
