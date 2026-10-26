# Driver ADG715 — Hito 1C

`ADG715.h/.cpp` controla el dispositivo con `TwoWire&` y dirección configurable
de 7 bits, por defecto `0x48`. No inicializa Wire ni depende del terminal, CN0349,
presets u otros drivers. Sin memoria dinámica ni retardos.

Referencia: [datasheet oficial ADG714/ADG715 Rev. E](https://www.analog.com/media/en/technical-documentation/data-sheets/adg714_715.pdf),
sección Theory of Operation y figuras 24–26. Escribe el byte de control con
START, SLA+W, un byte, ACK y STOP. `readState()` usa Receive Byte: SLA+R, un byte,
NACK del maestro y STOP. No hay puntero de registro. El readback da el byte del
registro de control: confirma la interfaz digital, no continuidad de contactos.

## Canales y API

La figura 24 asigna DB7…DB0 = S8…S1. Por tanto bit 0=S1, bit 7=S8; bit a 1 cierra,
bit a 0 abre. `0x12` cierra S2 y S5. Al encender, los ocho interruptores están
abiertos y el registro vale cero, según el datasheet.

- `ADG715(TwoWire&, address = 0x48)`; `address()`.
- `begin()` comprueba ACK sin escribir ni asumir el contenido.
- `readState(uint8_t&)` lee el registro de control real y deja intacto el destino si falla.
- `writeState(uint8_t)` acepta cualquier byte genérico, lo solicita y verifica por readback.
- `setSwitch(channel, enabled)` valida 1–8, lee el valor actual y cambia solo ese bit.
- `getSwitch(channel, enabled)` valida 1–8 y obtiene el bit de una lectura real.
- `getLastError()`, `lastError()`, `getLastWireError()` y `errorText()` dan diagnóstico.
- `hasRequestedState()/lastRequestedState()` y
  `hasConfirmedState()/lastConfirmedState()` separan solicitud de valor confirmado.

`writeState()` devuelve `true` solo si la escritura obtiene ACK y el registro leído
coincide. La solicitud queda registrada incluso si falla la escritura; la última
confirmación solo cambia tras coincidencia. Fallo de lectura o desajuste no
reemplaza la lectura actual con caché.

## Errores y límites

Se distinguen dirección/canal inválidos, buffer, NACK de dirección/datos, timeout,
error de bus, lectura incompleta y readback desigual. `requestFrom()` del core
reporta longitud, no causa específica para una lectura corta. El último error
queda retenido hasta un siguiente error; una operación correcta no lo borra.

El bus se configura desde `main`; transacciones síncronas usan su timeout de
25 ms. No hay reintentos ni rollback. Una escritura puede haber actualizado el
registro aun si el readback falla. Estado leído no equivale a verificación de
ruta analógica ni calibración del sistema.

## Hito 1E

MuxApp expone a ImpedanceApp ping, lectura actual de máscara y consulta de máscara autorizada conocida, sin pasar el terminal al driver. La recuperación integrada no escribe en ADG715: conserva la máscara y solo la relee. Su readback valida el registro digital, no continuidad ni ruta analógica. Los errores de NACK, timeout, lectura corta y verify mismatch del driver permanecen tipados; ImpedanceApp mantiene el historial de subsistema. Ver [`test/hito1e.md`](../../test/hito1e.md).
