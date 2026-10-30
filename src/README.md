# Firmware

El firmware coordina la CN0349 desde ESP32-S3 con Arduino. `main.cpp` crea
una instancia de AD5934, ADG715, MuxApp, ImpedanceApp, CN0349Manager, Terminal
y BleApp. Inicializa Serial y Wire, aplica el timeout I²C e inicia controlador,
terminal y BLE. El `loop()` llama a sus funciones `update()` en ese orden.

El controlador comparte la adquisición y el Setup entre terminal y BLE. No
hay máquinas de adquisición paralelas: Pt100 y EC se secuencian en un ciclo.
Las solicitudes BLE se procesan en el loop; los callbacks no acceden al I²C.

- [cn0349](cn0349/README.md): aplicación, calibración, Setup y ciclos.
- [impedance](impedance/README.md): adquisición DFT y recuperación.
- [mux](mux/README.md): rutas autorizadas e interlock.
- [temperature](temperature/README.md) y [conductivity](conductivity/README.md):
  conversiones.
- [terminal](terminal/README.md) y [ble](ble/README.md): interfaces.

Los pines, direcciones y constantes están en [include](../include/README.md).
El [README principal](../README.md) describe el sistema y sus limitaciones.
