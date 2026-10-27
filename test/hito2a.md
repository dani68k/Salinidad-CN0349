# Validación — Hito 2A, CN0349Manager

Versión 0.7.0. Este plan distingue compilación de comportamiento físico.

## Terminal

1. `help`; comprobar familia `cn0349` y comandos heredados.
2. `cn0349 status`, `cn0349 config`, `cn0349 preset list`: valores solicitados,
   máscara observada y preset aplicado deben distinguirse; ninguna consulta debe
   cambiar hardware.
3. `cn0349 preset set CAL_HIGH_1K`: solicitud software, sin conmutación; volver a
   status y comprobar que aplicado/máscara física conserva el valor leído.
4. `cn0349 measure single`; consultar `cn0349 status` durante la operación y
   `cn0349 result` después. Resultado esperado: DFT RAW a 2000 Hz, con frecuencia
   efectiva, estado IDLE y outcome COMPLETE. Referencia histórica de magnitud
   aproximada 8250 para CAL_LOW_1K (no tolerancia de precisión).
5. Restaurar CAL_LOW_1K mediante `cn0349 preset set CAL_LOW_1K` y ejecutar
   `cn0349 measure sweep`; esperar 9 puntos entre 1000 y 3000 Hz y consultar
   `cn0349 result`.
6. Ejecutar `cn0349 measure stop` en idle: debe rechazar sin tocar hardware.
   Durante un barrido, `stop` debe preservar el prefijo válido y reportar CANCELLED.
7. Conectar segundo inicio mientras busy mediante API/prueba de integración:
   debe rechazarse sin cambiar configuración.
8. `cn0349 diag`, `status`, `recover`; luego verificar que comandos heredados
   `ad5934 ...` y `adg715 ...` continúan funcionando.
9. Cambiar preset/frecuencia con comandos heredados; consultar manager y comprobar
   sincronización de configuración software y máscara observada. Si se escriben
   registros raw AD5934, volver a diagnosticar antes de medir.

No hay resultados físicos reportados para firmware 0.7.0.

## Automatizadas

La compilación PlatformIO ESP32-S3 terminó `SUCCESS` en 7,58 s (RAM 27 208 bytes,
Flash 391 138 bytes). `pio test -e native` se intentó, pero no compiló: `gcc` y
`g++` no están disponibles en PATH. No hay pruebas host de integración. La API
no tiene aún fake de ImpedanceApp/TwoWire, por lo que busy,
rechazos, transición de outcome y sincronización con comandos heredados necesitan
prueba en placa o añadir seam/mock en una iteración de test.

## Criterios para cerrar 2A

Compilar sin duplicar drivers/motor; API con consulta cacheada, configuración,
start/cancel/result; comandos `cn0349`; comandos anteriores; diag/recover; DFT RAW.
La configuración debe mantener las validaciones de frecuencia y DDS del motor.
