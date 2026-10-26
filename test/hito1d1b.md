# Validación — Hito 1D.1b

## Compilación y límites

Firmware 0.4.3 / Hito 1D.1b. Compilación PlatformIO del entorno
`esp32-s3-devkitc-1-n32r8v`: `SUCCESS` en 16,11 s, RAM 22 088 bytes,
Flash 370 598 bytes.
Esto no valida la placa ni el resultado de medida. El preset por defecto tras
reinicio es `CAL_LOW_1K`; los cambios de preset permanecen solo en RAM.

Los cuatro presets de calibración seleccionables son `CAL_HIGH_100R` (`0x09`,
RFB=100 Ω, RCAL=100 Ω), `CAL_HIGH_1K` (`0x11`, RFB=100 Ω, RCAL=1 kΩ),
`CAL_LOW_1K` (`0x12`, RFB=1 kΩ, RCAL=1 kΩ) y `CAL_LOW_10K` (`0x22`, RFB=1 kΩ,
RCAL=10 kΩ), como indica la [tabla CN-0349 de
Analog Devices](https://www.analog.com/en/resources/reference-designs/circuits-from-the-lab/cn0349.html).
No usar estas instrucciones para conectar una celda desconocida ni extrapolar
valores DFT esperados.

## Pruebas lógicas

Tras cargar/reiniciar 0.4.3:

```text
info
ad5934 measure config
ad5934 measure start
ad5934 measure status
ad5934 measure result
```

Debe mostrar versión 0.4.3, preset `CAL_LOW_1K` y, si la adquisición concluye,
resultado asociado a ese preset.

Probar la configuración lógica sin conmutar el ADG715:

```text
adg715 read
ad5934 measure set preset CAL_HIGH_1K
ad5934 measure config
adg715 read
```

La configuración debe mostrar 0x11, RFB 100 Ω y RCAL 1000 Ω; ambas lecturas de
`adg715 read` deben coincidir porque `set preset` no toca hardware.
Para comprobarlo, iniciar con AD5934 en standby y ADG715 en una máscara
autorizada distinta del preset lógico nuevo; anotar y restaurar el estado
original si se interrumpe la validación.

Para cada preset autorizado, iniciar y leer inmediatamente ADG715 tras terminar
la medida:

```text
ad5934 measure start
ad5934 measure status
ad5934 measure result
adg715 read
```

Repetir usando `CAL_HIGH_100R`, `CAL_HIGH_1K`, `CAL_LOW_1K` y `CAL_LOW_10K` como
selección previa.
Comparar el resultado y la lectura ADG715 con el preset usado en esa misma
adquisición. Registrar REAL, IMAG, magnitud y elapsed tal como se observaron;
D1 debe ser válido. No exigir un valor DFT numérico predeterminado. La lectura
debe ser respectivamente `0x09`, `0x11`, `0x12` y `0x22`. Los pares 0x09/0x11
son los puntos del rango alto; 0x12/0x22 son los del rango bajo.

Prueba de independencia: configurar `CAL_LOW_1K`, cambiar manualmente a
`adg715 set CAL_HIGH_1K`, después iniciar medida. Debe aplicarse `0x12` y el
resultado debe registrar CAL_LOW_1K, porque el preset lógico de medida gobierna
el siguiente inicio.

Prueba de rechazo: ejecutar `ad5934 measure set preset MEASURE_HIGH` y verificar
mensaje de error y que `ad5934 measure config` conserve el preset anterior.
Durante una adquisición, intentar el cambio de preset; se debe rechazar como
ocupado, sin tocar ADG715 ni la configuración actual.

## Registro inicial aportado

Antes de 0.4.3, el usuario informó tres adquisiciones físicas CAL_LOW_1K:

| # | REAL | IMAG | Magnitud DFT | Estado |
|---:|---:|---:|---:|---|
| 1 | -3405 | -7513 | 8248.587 | reportada válida en 0.4.1 |
| 2 | -3404 | -7514 | 8249.086 | datos aportados en el prompt 1D.1b |
| 3 | -3404 | -7515 | 8249.996 | datos aportados en el prompt 1D.1b |

No hay aún resultados observados de 0.4.3 ni de CAL_HIGH_100R/CAL_HIGH_1K/
CAL_LOW_10K. Una lectura de máscara ADG715 no verifica eléctricamente la ruta
analógica. La salida
del firmware sigue siendo código DFT crudo, sin coeficientes de calibración ni
conversión a impedancia/conductividad.
