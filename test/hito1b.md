# Plan de aceptación — Hito 1B (firmware 0.2.2)

Compilación ejecutada con éxito. El usuario ha reportado ping, lecturas, dump y
clear en hardware; las escrituras de configuración y las pruebas de fallo siguen
pendientes. Registrar las salidas, versión, fecha, conexiones y resultado restante.
Las evidencias del Hito 1A no validan por sí solas el protocolo de registros nuevo.

## Resultado parcial recibido

El usuario observó que `help`, `ad5934 help`, `ad5934 ping`, `ad5934 dump` y
`clear` responden. Ping recibió ACK en `0x0D`; dump recorrió los 17 bytes.
Lecturas del dump: control `0x80=0x21`, `0x81=0x00`; Start Frequency `0x20C49B`;
Frequency Increment `0x000000`; Number of Increments `0x0000`; Settling Cycles
`0x000A`; Status `0x72`; Real `0x0F10`; Imaginary `0xDEC6`.

`0x2100` tiene nibble D15:D12=`0x2`, cuyo comando en la tabla 10 de Rev. E es
Start Frequency Sweep. Control Low `0x00` tampoco muestra el valor reservado D3=1
prescrito por Rev. E. La versión actual incluye `ad5934 standby`: lee Control,
conserva bits D11:D8, escribe únicamente el comando Standby (`0xB`) en Control
High, establece Control Low=`0x08` y verifica los dos bytes. No modifica rango/PGA,
no admite escritura RAW y no activa una adquisición. Corrección 0.2.2: también
normaliza Control Low a `0x08` si el nibble alto ya indica power-down (`0xA`), sin
escribir Control High ni despertar DDS. Por tanto `0xA000` debe quedar en `0xA008`.
Si la verificación falla, detener pruebas y releer Control. El ACK no identifica
el IC y los valores Real/Imaginary no son una medida de conductividad.

El usuario confirmó que `ad5934 write24 0x82 0x123456` fue rechazado con el
mensaje que exige power-down/standby y Control Low=`0x08`; el terminal indica que
no cambió el modo. Después, `ad5934 status` devolvió `0x72`, D1 válido y D2
incompleto, en concordancia con el dump. Ese rechazo se registró antes de añadir
el comando dedicado `ad5934 standby`; no cuenta como prueba de escritura ni de
restauración exitosa. Pendiente probar la nueva transición y leer Control por
separado con `ad5934 read16 0x80` y `ad5934 read 0x81`.

## Preparación y lectura

Compilar/cargar el entorno existente, abrir monitor a 115200 y comprobar
`info`: firmware `0.2.2`, hito `1B`, GPIO4/5, 100000 Hz. `monitor_filters = direct`
y `monitor_echo = true` siguen configurados. No se modifica ADG715.

Enviar individualmente y esperar la respuesta:

```text
help
ad5934 help
ad5934 ping
ad5934 standby
ad5934 read 0x8F
ad5934 status
ad5934 read16 0x80
ad5934 read16 0x88
ad5934 read24 0x82
ad5934 dump
```

El ping debe informar ACK en `0x0D`, sin asegurar identidad. La lectura de estado
debe ser coherente con `status`: D1 y D2 se decodifican y el resto se ignora.
Sin adquisiciones, D1=0 no es fallo. No exigir ceros en los registros de
configuración/resultados. Con Control reportado como `0x2100`, ejecutar
`ad5934 standby`: debe parar el sweep, preservar rango/PGA y verificar `0xB108`.
Después, comprobar `ad5934 read16 0x80`, `ad5934 read 0x81` y `ad5934 status`.
Tras alimentación completa, Rev. E indica control inicial `0xA008`, pero registrar
el valor observado en lugar de asumirlo.

El dump debe agrupar Control, Start Frequency, Frequency Increment, Number of
Increments, Settling Cycles, Status, Real e Imaginary; exactamente 17 bytes, sin
`0x8C–0x8E`, `0x90–0x93` u otras direcciones. No interpretar los resultados como
impedancia. Si es posible usar analizador lógico, comprobar selección `B0, reg`
con STOP, lectura de un byte con NACK final y STOP; repetir por cada dirección.

## Escritura, orden de bytes y restauración

Usar Start Frequency, cuyos 24 bits son de configuración y permiten restaurar el
valor completo. Antes de empezar, leer y **anotar fuera del terminal** el valor
de `ad5934 read24 0x82` como ORIGINAL y el de `ad5934 read16 0x80` como CONTROL.
No continuar si una lectura falla. Ejecutar primero `ad5934 standby` y verificar
después con `ad5934 read16 0x80` que el control es `0xA008` o un modo standby
`0xB?08` apropiado. La aplicación solo permite la escritura si detecta modo
power-down/standby y Low=`0x08`.

Secuencia mínima para el estado reportado `Control High=0xA0`, `Control Low=0x00`:

```text
ad5934 read24 0x82
```

Anotar la respuesta como ORIGINAL antes de cualquier cambio. Después:

```text
ad5934 standby
ad5934 read16 0x80
ad5934 write24 0x82 0x123456
ad5934 read24 0x82
```

Se espera `ad5934 standby` verifique `0xA008` sin cambiar power-down; la lectura
de 16 bits debe confirmarlo. La escritura debe informar que preserva el modo y
verificar que la nueva frecuencia de inicio es `0x123456`. Si se rechaza, falla
una transacción o difiere la relectura, detenerse; no continuar con escrituras.
Restaurar ORIGINAL al terminar usando el procedimiento siguiente.

```text
ad5934 write24 0x82 0x123456
ad5934 read24 0x82
ad5934 read16 0x82
ad5934 read 0x84
ad5934 write16 0x82 0x2345
ad5934 read24 0x82
ad5934 write 0x84 0x67
ad5934 read24 0x82
```

Resultados esperados: cada escritura informa relectura verificada; primeras
lecturas `0x123456`, `0x1234` y `0x56`; después `0x234556` y `0x234567`.
Esto ejercita escrituras de 24, 16 y 8 bits y orden MSB primero sin activar DDS.

**Restaurar al terminar:** ejecutar `ad5934 write24 0x82 ORIGINAL`, sustituyendo
ORIGINAL por el hexadecimal previamente anotado (no escribir la palabra literal).
Releer `0x82` a 24 bits y comprobar igualdad. Releer Control a 16 bits y comprobar
que sigue siendo CONTROL. No sustituir el original por cero: estos registros no
tienen un valor de arranque garantizado.

Si cualquier escritura o verificación falla, detener la secuencia y registrar
el error. Puede haber cambios parciales. Recuperar comunicación y releer estado,
control y configuración antes de intentar restaurar el original en modo permitido.
No continuar ensayos sobre una configuración no confirmada.

## Operaciones que deben rechazarse

| Comando | Resultado esperado |
| --- | --- |
| `ad5934 read 0x8C` | Registro reservado/no documentado |
| `ad5934 read16 0x8B` | Fuera de grupo; no atravesar `0x8C` |
| `ad5934 read24 0x83` | Cruza grupo, aunque las direcciones existan |
| `ad5934 write 0x8F 0x00` | Solo lectura |
| `ad5934 write16 0x94 0x1234` | Solo lectura |
| `ad5934 write 0x80 0x10` | Control protegido; no iniciar DDS |
| `ad5934 write16 0x80 0x2008` | Control protegido; no iniciar barrido |
| `ad5934 write 0x81 0x18` | Control protegido; no reset |
| `ad5934 write 0x80 0xB1` | Control protegido; usar el comando dedicado `ad5934 standby` |
| `ad5934 write 0x88 0x02` | Bits no permitidos |
| `ad5934 write16 0x88 0x0200` | Fuera de rango de 9 bits |
| `ad5934 write16 0x8A 0x0401` | Multiplicador reservado |
| `ad5934 write 0x8A 0x08` | Bits altos no permitidos |
| `ad5934 write 0x82 0x100` | Desbordamiento de 8 bits |
| `ad5934 write16 0x88 0x10000` | Desbordamiento de 16 bits |
| `ad5934 write24 0x82 0x1000000` | Desbordamiento de 24 bits |
| `ad5934 read 0x180` | Dirección fuera de 8 bits |
| `ad5934 read -1` / `ad5934 read 0x` / `ad5934 read zz` | Hexadecimal inválido |
| `ad5934 read` / `ad5934 ping extra` | Número de argumentos inválido |

Ninguna operación rechazada debe enviar una escritura de registro. Comprobar
que `help` responde después de cada error. Con analizador lógico, verificar que
validaciones estáticas fallidas no generan tráfico I²C. Para comprobar rechazo
por modo activo, usar un futuro mock o montaje expresamente preparado: no se
habilita adquisición desde esta versión para provocar la prueba.

## Errores I²C y regresión

1. Con un montaje controlado sin esclavo y líneas con condiciones eléctricas
   correctas, probar `ad5934 ping` y `ad5934 read 0x8F`: deben informar NACK de
   dirección y recuperar el terminal. Reconectar correctamente y repetir ping.
2. Con inyección de fallos/mocks o analizador apropiado, comprobar NACK de datos,
   timeout de transmisión y lectura incompleta. Wire permite distinguir el
   timeout de `endTransmission`; un fallo de recepción se muestra como lectura
   incompleta sin atribuirle una causa que `requestFrom` no ofrece.
3. Fallo en el segundo/tercer byte: abortar y conservar la variable de lectura;
   en escritura, informar fase y bytes completados sin anunciar éxito ni intentar
   rollback automático. Esta prueba queda pendiente de un montaje de fallos/mocks.
4. Enviar en ráfaga `i2c scan` seguido de `ad5934 read 0x8F`: rechazar la segunda
   operación si el escaneo sigue activo. Enviar `ad5934 dump`, `i2c scan` y otro
   comando AD5934: rechazar E/S solapada mientras dure el dump. `help`/`info`
   deben seguir disponibles. El dump se interrumpe ante el primer error.
5. Repetir los cinco comandos 1A, desconocidos, entradas de 95/96 caracteres,
   recuperación, mayúsculas y CR/LF/CRLF según `test/README.md`. El `help` global
   añade una entrada `ad5934 help`; no elimina los cinco anteriores.

## Cierre

El usuario declaró el Hito 1B finalizado. En esta conversación quedaron reportados
en hardware `help`, `ad5934 help`, `ad5934 ping`, `ad5934 dump`, `ad5934 status`,
`clear` y el rechazo seguro de una escritura con el control en modo no permitido.
La compilación 0.2.1 terminó correctamente. No se incluyen resultados inventados
para pasos sin salida compartida, entre ellos la verificación física posterior a
`ad5934 standby` o la secuencia de escritura/restauración.

No se implementó Hito 1C, el driver ADG715 ni medición de conductividad.
