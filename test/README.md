# Validación de Hitos 1A, 1B, 1C, 1D y 1E

Este directorio contiene procedimientos manuales y pruebas unitarias host
(`test_ad5934_math`). Las pruebas físicas reportadas se registran a continuación.
Registrar cada prueba con fecha, revisión, placa, conexiones, salida observada y
resultado; no marcarla superada por compilar.

## Compilación

```sh
pio run -e esp32-s3-devkitc-1-n32r8v
```

Esperar `SUCCESS` y la generación de `firmware.elf` y `firmware.bin` en
`.pio/build/esp32-s3-devkitc-1-n32r8v/`. Consultar el resultado real del desarrollo
en el diario raíz. La compilación no valida E/S ni dispositivos.

Para el Hito 1C, usar [test/hito1c.md](hito1c.md). La compilación 0.3.0 y las
primeras pruebas físicas ADG715 reportadas están en el diario y ese plan;
rechazos, interlock inseguro, restauración y validación analógica siguen
pendientes y se registran por separado.

Para Hito 1D.1, consultar [test/hito1d1.md](hito1d1.md): incluye la suite
Unity nativa, límites, secuencia DFT, cinco repeticiones y recuperación. La
compilación ESP32 pasó; las pruebas host no corrieron por ausencia de gcc/g++ y
la validación física queda pendiente.

Para el barrido 1D.2, consultar [test/hito1d2.md](hito1d2.md). Las pruebas host
se ampliaron para sus cálculos matemáticos, pero en este equipo no hay `gcc` ni
`g++`; la compilación del firmware ESP32 no sustituye esos tests ni la prueba
física de barrido.

## Resultados físicos reportados — 2026-10-26

El usuario ha compartido salidas de firmware 0.1.0, Hito 1A: `help` e `info`
correctos; escaneo con ACK en `0x0D` y `0x48`, 112 sondeos, 110 NACK y 0 errores;
`i2c status` conserva los resultados y la configuración esperada.

`clear` mostró `␛[2J␛[H` sin limpiar. Se añadió `monitor_filters = direct` a
`platformio.ini`. Cerrar y reabrir el monitor en el terminal integrado de VS Code
y repetir `clear`; no requiere recompilar ni cargar el firmware. Esta repetición
y las demás pruebas no reportadas siguen pendientes. El filtro debe aparecer como
`direct` en la cabecera del monitor.

El usuario confirmó que `clear` funciona con ese filtro. Las salidas físicas
registradas de `help`, `info`, `i2c scan` e `i2c status` corresponden al firmware
0.1.0 y verifican la infraestructura 1A, no el driver AD5934 nuevo.
Para 0.2.2, usar el [plan del Hito 1B](hito1b.md); sus pruebas físicas siguen
pendientes.

## Procedimiento de pruebas físicas

Cargar el firmware con PlatformIO, abrir el monitor a 115200 y usar una
terminación CR, LF o CRLF. Si hace falta seleccionar puerto, consultar el README
raíz. Mantener SDA GPIO4, SCL GPIO5 y masa común con la placa.

| Prueba | Acción | Criterio de aceptación |
| --- | --- | --- |
| Arranque | Reiniciar con monitor abierto | ATLANTES CN-0349, 0.1.0, 1A, GPIO4/5, 100000 Hz y sistema preparado; si falla Wire, error explícito. |
| Ayuda | `help` | Los cinco comandos con descripción. |
| Información | `info` | Identidad y parámetros correctos. |
| Estado inicial | `i2c status` antes de escanear | Inicialización y ningún diagnóstico completado. |
| Escaneo | `i2c scan` | Solo ACK reales; direcciones hexadecimales; 112 sondeos y totales coherentes. No exigir dos dispositivos. |
| Estado posterior | `i2c status` | Totales del último escaneo y última transacción; último error si lo hubo. |
| Repetición | Nuevo `i2c scan` | Contadores reiniciados y resultado nuevo; el anterior se conserva hasta completar. |
| Uso durante escaneo | Enviar `i2c scan`, `info`, `i2c status` y otro `i2c scan` en una ráfaga | Información y progreso disponibles; segundo escaneo rechazado si el primero sigue activo. |
| Bus sin dispositivos | Repetir con ningún esclavo, manteniendo condiciones eléctricas correctas | Cero ACK es válido; NACK separados de errores. |
| Desconocido | `desconocido`, luego `help` | Error claro y terminal todavía operativo. |
| Argumentos extra | `info extra` | Comando desconocido; no ejecuta `info` por prefijo. |
| Mayúsculas y espacios | `  I2C   STATUS  ` y `HeLp` | Mismo resultado que los comandos normalizados. |
| Terminaciones | Enviar `info` con CR, LF y CRLF, también CR/LF separados en el tiempo | Una respuesta por comando. |
| Líneas vacías | CR/LF y líneas de espacios | Ningún comando ni error. |
| Límite exacto | `help` seguido de 91 espacios y fin de línea (95 caracteres) | Se ejecuta una vez, sin error de longitud. |
| Desbordamiento | `help` seguido de 92 espacios y fin de línea (96 caracteres) | Un error de longitud; no ejecuta `help`. |
| Recuperación | Enviar más de 200 caracteres, fin de línea, luego `help` | Línea larga descartada; el siguiente comando funciona. |
| Retroceso | Enviar `helx`, byte BS, `p` y fin de línea | Ejecuta `help` (sin eco visual incorporado). |
| Limpieza | `clear` | Limpia y sitúa cursor al inicio si el monitor soporta ANSI. |

Para generar líneas exactas, se pueden usar expresiones Python como
`"help" + " " * 91 + "\r\n"`; enviarlas con un terminal capaz de transmitir bytes
sin añadir caracteres. Si el monitor edita líneas localmente, los tests de bytes
de control requieren desactivar esa edición o utilizar otro emisor serie.

Para comprobar errores de comunicación, utilizar un montaje de prueba controlado
o una futura simulación de `TwoWire`: códigos 1, 3, 4, 5 o desconocidos deben
contarse como errores, no como dispositivos. Un fallo de `Wire.begin()` debe
dejar `help`, `info`, `i2c status` y `clear` disponibles y rechazar `i2c scan`.
Desconectar un esclavo normalmente produce NACK; no demuestra un timeout ni
un fallo de inicialización. Estas rutas no se han validado físicamente.

Para Hito 1E, usar [`hito1e.md`](hito1e.md): modelo de errores, diagnóstico, stop, recuperación y pruebas de fault paths. En 0.6.0 el build ESP32 compila; el test nativo se intentó pero gcc/g++ no están disponibles y las pruebas físicas están pendientes.

Para Hito 2A, usar [`hito2a.md`](hito2a.md) para API/terminal manager, resultados
DFT y compatibilidad con comandos heredados. El build ESP32 compila; no hay aún
mock de aplicación/TwoWire ni validación física 0.7.0.

Para Hito 2B, consultar [`hito2b.md`](hito2b.md). La compilación ESP32-S3 pasó; las mediciones de calibración en placa y la repetibilidad permanecen pendientes. La salida de compilación no valida GF ni la ruta analógica.

## Hito 2C.1

[Procedimiento físico y matriz de resultados pendientes](hito2c1.md).
[Suite host MSVC con I²C simulado](host_cn0349/README.md): tres grupos PASS;
PlatformIO native no ejecutable por falta de gcc/g++. La simulación no valida
resistencias externas ni exactitud analógica.

[Hoja de trabajo: cinco medidas por combinación, salidas y comentarios](hito2c.1_testing.md).

Estado 2026-10-27: validación física 2C.1 cerrada con la serie de veinte medidas y sus límites en [hito2c.1_testing.md](hito2c.1_testing.md).

## Hito 2D — 2026-10-27

[Pruebas Pt100 puras](test_pt100/README.md): tres grupos Unity, ocho referencias
IEC completas, continuidad y errores. [Suite host](host_cn0349/README.md): cuatro
grupos con regresiones y temperatura, errores propagados y calibración inmutable.
`host_cn0349\run_math.cmd` ejecuta las suites Unity Pt100 y AD5934 con MSVC:
14 pruebas PASS. `host_cn0349\run.cmd`: cuatro grupos PASS.
PlatformIO native sigue sin gcc/g++; no se afirma PASS por esa vía.
[Protocolo físico y evidencias software](hito2d.md): validación física pendiente.

Preparación automática de arranque, 2026-10-27: `host_cn0349\run.cmd` amplía
la cobertura a cinco grupos PASS. Incluye recover seguido de cuatro calibraciones,
medida posterior, cancelación y fallos. En placa queda pendiente verificar que
tras un encendido aparece el anuncio de preparación completa, los cuatro slots
de `cn0349 cal list` son válidos y `cn0349 temperature start` inicia una medida
sin preparación manual. Registrar salida de arranque completa y nuevos IDs/GF;
los valores deben proceder de cada calibración, sin ajustes empíricos.

Hito 2E.1, 2026-10-27: [plan de 40 adquisiciones y análisis pendiente](hito2e1.md).
La suite host incorpora EC HIGH/LOW y conserva regresiones RTD/temperatura:
siete grupos PASS. La compilación ESP32-S3 es SUCCESS. No se presenta una
caracterización eléctrica hasta ejecutar la matriz en placa y revisar R2.

Registro físico 2E.1: [40 adquisiciones A–H y conclusión](hito2e1.md).

Hitos 2E.2/2E.3, 2026-10-27: [protocolo para sonda EC real y estimación de
conductividad](hito2e2.md). Software host y compilación ESP32-S3 completados;
ensayos físicos con líquidos pendientes. 2E.4 reserva patrones y validación.
