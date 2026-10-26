# Plan de validación — Hito 1C

La compilación del firmware 0.3.0 terminó correctamente. El usuario ha reportado
las primeras pruebas físicas de comandos y conmutación, registradas abajo. Las
pruebas restantes siguen pendientes. El readback valida el registro ADG715, no
la conducción analógica.

## Preparación

1. Cargar `esp32-s3-devkitc-1-n32r8v`, abrir monitor a 115200 y confirmar `info`:
   versión `0.3.0`, hito 1C, GPIO4/5, bus 100 kHz.
2. Verificar alimentación, nivel lógico y conexiones de la placa antes de
   conmutar las rutas analógicas.
3. Ejecutar `ad5934 read16 0x80` y `ad5934 read 0x81`. Debe estar en
   power-down/standby y Control Low=`0x08`. Si no, detenerse y normalizar con
   el procedimiento 1B. MuxApp comprueba esta condición antes de cada escritura.
4. Ejecutar `adg715 read` y anotar literalmente el byte como `ORIGINAL` antes
   de cualquier escritura; guardar también `adg715 status`.

## Lectura y presets

```text
adg715 help
adg715 ping
adg715 read
adg715 status
adg715 presets
```

Esperar ACK en `0x48`. Read/status deben informar el byte leído real y los ocho
bits (bit 0=S1; bit 7=S8). `presets` debe listar las ocho máscaras autorizadas,
incluida la calibración alta de 100 Ω (`CAL_HIGH_100R = 0x09`). `ping` no debe
modificar el estado.

## Aplicar y restaurar presets

Antes de cambiar nada, guardar el byte original como `ORIGINAL`. Confirmar de
nuevo que el AD5934 está en power-down/standby y Control Low=`0x08`. Enviar
comandos uno por uno:

```text
adg715 set CAL_LOW_1K
adg715 read
adg715 status
adg715 set MEASURE_HIGH
adg715 read
```

Esperar readback `0x12` (S2/S5) y después `0x81` (S1/S8). Status muestra preset,
estado leído, última solicitud, último estado confirmado y error retenido. Cada
cambio se envía como un byte completo; no hay transición ALL_OFF automática.

Restaurar con `adg715 write 0xNN` usando el byte anotado solo si `ORIGINAL`
coincide con una máscara autorizada; el comando no acepta literalmente la palabra
`ORIGINAL`. Si el estado inicial no está autorizado, no se puede restaurar con
la interfaz protegida: detener la prueba y acordar el preset seguro de reposo
antes de cambiarlo. Leer luego y confirmar la restauración. No enviar máscaras
arbitrarias.

## Resultados físicos reportados

El usuario ejecutó `adg715 help`, `ping`, `status`, `set CAL_LOW_1K`, `read`,
`status`, `set MEASURE_HIGH` y `read`. `ping` obtuvo ACK en `0x48`. El estado
inicial reportado fue `0x00` (ALL_OFF); `CAL_LOW_1K` produjo readback `0x12`
(S2/S5 ON), y `MEASURE_HIGH` produjo readback `0x81` (S1/S8 ON). `status`
confirmó el byte actual y los campos solicitado/confirmado en `0x12`. El readback
final fue `0x81`.

Esto valida la comunicación y los valores del registro para esos dos presets.
No se aportó aún la restauración del estado original `0x00`, rechazo de máscaras
no autorizadas, comportamiento con interlock inseguro, prueba de fallo I²C ni
validación analógica. No se puede restaurar `0x00` mediante la interfaz actual,
porque no es un preset permitido para `write`; acordar primero un estado de
reposo permitido si se desea restaurar desde `0x81`.

## Rechazos y regresión

| Prueba | Resultado esperado |
| --- | --- |
| `adg715 write 0x13` | Máscara no autorizada; ninguna escritura I²C |
| `adg715 set NOT_A_PRESET` | Preset desconocido; sin escritura |
| `adg715 switch 1 on` | Rechazado por la aplicación CN0349 |
| `adg715 switch 9 off` | Rechazado; no escribe |
| `adg715 set` / argumentos extra | Error claro; terminal sigue operativo |
| AD5934 activo o Low distinto a `0x08`, seguido de preset | Interlock rechaza antes de escribir al ADG715 |
| Error al leer el interlock AD5934 | Se informa error; no se escribe multiplexor |
| Montaje controlado sin ADG715: `ping` / `read` | NACK o lectura incompleta informados; terminal recuperable |
| Reconectar y `adg715 read` | Estado obtenido del registro actual, sin usar caché |
| API genérica `setSwitch(0/9, true)` | `InvalidChannel`; no transacción |
| Hitos anteriores | Comandos generales e `ad5934` siguen respondiendo |

Si es posible, usar analizador lógico para confirmar la escritura de un byte y la
lectura de un byte. No presentar readback ni preset como validación de continuidad,
calibración o exactitud analógica; esas pruebas quedan para un hito posterior.
