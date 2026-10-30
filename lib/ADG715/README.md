# Driver ADG715

Controla el registro de interruptores mediante `ADG715(TwoWire&, address)`;
dirección por defecto 0x48. No inicializa Wire ni conoce las rutas CN0349.

- `begin()` comprueba ACK sin modificar el estado.
- `readState(state)` lee el byte de control real.
- `writeState(state)` escribe el byte y verifica su readback.
- `setSwitch(channel, enabled)` y `getSwitch(channel, enabled)` operan sobre
  canales 1–8. Las operaciones devuelven `bool`.
- `lastError()`, `getLastWireError()` y `errorText(error)` informan fallos;
  los accesores de estado solicitado y confirmado distinguen ambos valores.

Los bits 0–7 corresponden a S1–S8; un bit a 1 cierra el interruptor. El driver
acepta máscaras genéricas; MuxApp aplica la política de rutas autorizadas e
interlock. Una escritura puede haber cambiado el registro si falla el readback;
no hay rollback automático. La lectura digital no demuestra continuidad de
contactos o funcionamiento analógico.

Referencia: [datasheet ADG715](https://www.analog.com/media/en/technical-documentation/data-sheets/adg714_715.pdf).

Las interfaces y tipos públicos de `ADG715.h` forman parte de la referencia
HTML generada con `doxygen Doxyfile` desde la raíz del proyecto.
