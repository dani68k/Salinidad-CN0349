# Validación — Hito 1E

Firmware 0.6.0. Este plan separa compilación, pruebas host y validación física.
No se requieren desconexiones de la placa durante adquisiciones.

## Comandos nuevos

```text
ad5934 diag
ad5934 status
ad5934 errors
ad5934 errors clear
ad5934 recover
```

Ejecutar `ad5934 diag` dos veces. Comparar Control, Status y máscara ADG715;
ningún preset ni adquisición debe cambiar. Confirmar que ACK se presenta como
comunicación y no como identidad/ruta analógica. Ejecutar `ad5934 status` y
verificar busy, readiness, máscara, puntos, errores, D1/D2. `errors clear` solo
debe limpiar historial si el subsistema no está ocupado y no conserva error activo.

## Pruebas físicas pendientes

1. Diagnóstico con ambos dispositivos conectados y repetido sin cambios.
2. `ad5934 measure sweep stop` inactivo: respuesta sin escrituras.
3. Iniciar barrido y cancelar: standby leído, puntos parciales preservados y
   terminal utilizable.
4. Parámetros inválidos: rechazo sin iniciar hardware ni perder resultados.
5. Durante adquisición, intentar preset, frecuencia, escritura y segunda medida;
   confirmar rechazo y ninguna operación incompatible.
6. Timeout DFT y NACK/errores de lectura/escritura/readback: comprobar código,
   contador, último/activo y recuperación sin declarar éxito falso.
7. Fallo/readback ADG715, y `ad5934 recover` tras condición recuperable: máscara
   preservada, AD5934 en standby confirmado, error activo limpiado pero último
   error conservado.
8. Tras recovery, realizar medida única y barrido; comprobar D1, D2, resultados
   y standby.
9. Confirmar que errores no borran los puntos válidos previos.

## Resultados físicos iniciales reportados — 2026-10-26

El usuario ejecutó `ad5934 diag`: ACK AD5934/ADG715, control seguro, configuración
relevante válida, Status `0x72` y máscara `0x22` conocida; resultado READY.
`ad5934 status` reflejó READY, estado IDLE y NONE en errores. `ad5934 errors`
mostró contadores en cero y `ad5934 errors clear` confirmó limpieza sin tocar
hardware. A continuación `ad5934 measure sweep start` completó 9/9 puntos y
reportó standby. La adquisición configuró CAL_LOW_1K (`0x12`) partiendo del
estado diagnóstico `0x22`. Finalmente, `ad5934 recover` verificó standby y
conservó la máscara actual.

Esta es una validación física inicial de las rutas normales de diagnóstico,
estado, clear, barrido y recovery. No se reportó una segunda ejecución de diag;
tampoco cubre errores inyectados, cancelación, fallo de readback ni medición
única posterior a recovery. La lectura de máscara no verifica la ruta eléctrica.

## Pruebas automatizadas

Se añadió prueba Unity del nombre de los códigos tipados y valores iniciales del
registro de error. `pio test -e native` no compiló en el entorno actual:
`gcc` y `g++` no están instalados/en PATH. Falta una capa mock de `TwoWire` para
probar inyección de NACK, timeout y verify mismatch sin tocar firmware de
producción. La compilación ESP32 valida integración, no esas rutas.

## Compilación

`pio run -e esp32-s3-devkitc-1-n32r8v`: `SUCCESS`, 6,95 s, RAM 25 360 bytes,
Flash 383 418 bytes. Hay límite de 1500 ms para fases de control/configuración,
500 ms para DFT y 25 ms por transacción Wire. No es validación física.
