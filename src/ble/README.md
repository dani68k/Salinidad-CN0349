# BLE GATT — Hito 3A

`BleApp` expone en modo periférico el estado de `CN0349Manager`; no contiene
lógica de calibración o medida. `begin()` crea GATT y advertising, y `update()`
compara un JSON compacto con el anterior. Solo cambia la característica y
notifica cuando cambia algún campo. No usa `delay`, tareas propias ni Serial.

- Nombre anunciado: `CN0349`.
- Servicio ATLANTES: `8d7b0001-6f7a-4f31-9c4d-2b6e6a7f3a10`.
- Característica STATUS: `8d7b0002-6f7a-4f31-9c4d-2b6e6a7f3a10`.
- Propiedades STATUS: READ + NOTIFY.

Formato UTF-8: `{"state":"READY","cal":4,"busy":false}`. `cal` es el número
real de calibraciones legacy válidas (0–4); `busy` procede de `isBusy()`.
NotInitialized se publica como INIT, Calibrating como CALIBRATING, Idle listo
como READY, Measuring/Stopping como MEASURING y cualquier error/no readiness
como ERROR. NimBLE reanuda advertising al desconectar.

Dependencias: CN0349Manager y NimBLE-Arduino 2.x. Limitaciones 3A: sin escritura,
órdenes, seguridad, bonding, reconexión de cliente, medidas ni configuración BLE.
