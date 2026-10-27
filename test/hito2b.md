# Pruebas Hito 2B — calibración electrónica CN-0349

Versión de firmware: 0.8.0. Fecha de implementación: 2026-10-26. No cargar como validada ninguna prueba física hasta registrar la salida real del terminal.

## Validación de software

- PlatformIO ESP32-S3: `SUCCESS` en 7,67 s; RAM 27 912 bytes, Flash 401 134 bytes.
- `pio test -e native`: intentado; no compiló porque `gcc`/`g++` no están disponibles en el entorno.
- Compilar no valida lectura de DFT, factor GF ni ruta analógica.

## Procedimiento físico

Precondición: CN-0349 original con AD5934 `0x0D`, ADG715 `0x48`, MCLK externo 1 MHz, alimentación y bus ya validados. Arrancar y ejecutar `ad5934 diag`; debe informar READY. Si hay una operación activa, esperar o cancelarla antes de empezar.

1. `cn0349 cal help`, `cn0349 cal status`, `cn0349 cal list` y `cn0349 cal result`. Debe mostrarse ayuda, estado sin actividad y cuatro entradas no disponibles al arrancar limpio.
2. Probar `cn0349 cal run INVALID`; debe rechazarse sin conmutar hardware. Verificar `cn0349 cal status` y que no hay calibración activa.
3. Para API: probar preset enum fuera de rango y frecuencia 0 / fuera del rango aceptado; deben devolver `InvalidArgument`. Probar busy durante single/sweep/cal; debe devolver `Busy` sin apropiarse del motor.
4. Ejecutar individualmente y esperar estado COMPLETE para cada uno: `cn0349 cal run CAL_HIGH_100R`, `CAL_HIGH_1K`, `CAL_LOW_1K`, `CAL_LOW_10K`. Consultar `cn0349 cal status`, `result` y `list`. Cada serie requiere 5 D1 válidos, preset/máscara confirmados, 2000 Hz, AD5934 en standby, GF finito positivo y estadísticas visibles.
5. Comparar magnitudes con referencias orientativas, no umbrales: HIGH_100R ~8191, HIGH_1K ~859, LOW_1K ~8251 y LOW_10K ~839. Diferencias requieren investigar montaje/ruta, no forzar éxito ni sustituir constantes.
6. Repetibilidad: repetir cada preset; comparar muestras, fase circular, desviación y variación relativa. Registrar valores observados y decidir tolerancias experimentales aparte. Ningún valor atípico se elimina automáticamente.
7. Conservación ante fallo: después de una calibración válida, provocar fallo controlado durante nueva serie si se dispone de método seguro. Debe quedar disponible el slot anterior. No desconectar componentes durante una transacción activa.
8. Cancelación: iniciar calibración, enviar `cn0349 cal stop` durante adquisición; esperar estado CANCELLED y standby. Verificar que los slots previos no cambiaron.
9. `cn0349 cal run all`; debe procesar los cuatro presets secuencialmente, 20 adquisiciones en total, sin bloquear comandos `help`, `cal status`, `cal list`. Los cuatro resultados deben aparecer válidos al finalizar.
10. `cn0349 cal clear`; los slots pasan a unavailable y el estado hardware no se altera. Repetir clear durante calibración: debe rechazarse.
11. Regresión: `cn0349 measure single`, `cn0349 measure sweep`, `cn0349 diag`, `ad5934 measure start` y comandos de lectura ADG715; deben conservar comportamiento y standby. Probar stop cuando no hay actividad.
12. Compatibilidad: cambiar frecuencia/configuración por las APIs existentes y consultar `cal list`; el coeficiente debe seguir visible con `compatible=no`, nunca reutilizarse silenciosamente.

## Registro de resultados físicos

Estado a la fecha de implementación: pendiente. Registrar para cada ejecución firmware/placa, preset, máscara confirmada, REAL/IMAG de las cinco muestras, magnitudes, fases, media, desviación, variación relativa, GF, duración, standby y errores. La lectura digital del ADG715 confirma el registro, no continuidad ni exactitud de la ruta analógica.

## Alcance y limitaciones

GF usa `1/(RCALnominal × magnitud media)` bajo hipótesis de cadena lineal/estable y modelo DFT compatible. RFB y RCAL tienen tolerancia. Estos ensayos electrónicos no son calibración de celda ni validación de impedancia externa. No interpretar fase DFT como fase física. No pasar al Hito 2C ni declarar precisión metrológica sin revisar resultados y diseñar esa validación.

## Evidencia física reportada — 2026-10-26

Calibración individual `CAL_HIGH_100R`: cinco muestras válidas; media 8194,710; desviación muestral 0,917; variación 0,011 %; fase media −114,315°; GF 0,000001220; frecuencia efectiva 1999,996 Hz; estado COMPLETE y standby.

Segunda ejecución `cn0349 cal run all`: COMPLETE, cuatro presets, cinco D1 válidos por preset (20 en total), sin error; cada medida reportó standby. Magnitudes DFT observadas: HIGH_100R 8195,987 / 8194,897 / 8194,310 / 8193,811 / 8192,813; HIGH_1K 859,142 / 859,559 ×4; LOW_1K 8249,826 / 8250,653 / 8249,742 / 8250,155 / 8249,245; LOW_10K 838,955 ×5. `cal list` informó todos válidos/compatibles con GF redondeados 0,000001220 / 0,000001164 / 0,000000121 / 0,000000119. `cal result` de LOW_10K: media 838,955, desviación y variación mostradas como 0,000, fase −114,732°, duración 1142 ms.

Las magnitudes de LOW_10K idénticas y desviación cero reflejan igualdad a la resolución observada, no ruido cero. Las cuatro medias son cercanas a los valores históricos orientativos. No son límites de aprobación. Esta evidencia acredita la secuencia digital/DFT con las resistencias internas; no demuestra precisión analógica ni calibración de celda. No se reportaron ensayos físicos de cancelación, fallo/retenimiento, clear ni reinicio.

**Estado:** calibración electrónica de los cuatro presets confirmada en hardware; validación de exactitud externa sigue fuera de alcance. El Hito 2C no se inicia aquí.
