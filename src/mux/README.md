# MuxApp — control seguro del CN-0349 (Hito 1C)

Actualización 2026-10-27: `selectPreset`, `applyPt100` y el método interno
`setAuthorized` admiten `quiet=false`. La adquisición continua activa esta opción
para omitir solo los mensajes informativos de ruta ya confirmada o escritura
correcta. Las lecturas, validaciones, escritura/readback y mensajes de error
siguen ejecutándose. Los comandos manuales conservan su salida por defecto.

Hito 2E.1, 2026-10-27: `ecInfo(mask, info)` limita las nuevas solicitudes a
`0x81` (MEASURE_HIGH, RFB 100 ohm) y `0x82` (MEASURE_LOW, RFB 1 kohm).
`applyEc(mask)` reutiliza `selectPreset`, con lectura previa y verificación
posterior del ADG715. La tabla de máscaras ya existía. El readback solo confirma
estado digital; consultar [modelo eléctrico](../../docs/ec-route.md) antes de
interpretar R2, continuidad analógica o resistencia externa.

`MuxApp` adapta el driver ADG715 a las rutas documentadas de EVAL-CN0349-PMDZ.
Solo acepta máscaras autorizadas y lee el AD5934 antes de conmutar: exige
power-down (`0xA`) o standby (`0xB`) y Control Low=`0x08`. No modifica el AD5934.
El driver multiplexor no contiene lógica específica del CN0349.

## Comandos

```text
adg715 help
adg715 ping
adg715 read
adg715 status
adg715 presets
adg715 set CAL_HIGH_100R
adg715 set CAL_HIGH_1K
adg715 set CAL_LOW_1K
adg715 set CAL_LOW_10K
adg715 set RTD
adg715 set MEASURE_HIGH
adg715 set MEASURE_LOW
adg715 off
adg715 write 0x12
adg715 switch 1 on        # rechazado en esta aplicación
```

Nombres no distinguen mayúsculas. `write` solo acepta una máscara hexadecimal
de 8 bits incluida exactamente en la lista autorizada. Presets desconocidos,
máscaras ajenas y comandos `switch` se rechazan sin escribir. `help` y `presets`
no necesitan acceso I²C.

| Preset | Máscara | Ruta CN0349 documentada |
| --- | --- | --- |
| `CAL_HIGH_100R` | `0x09` | RFB=R9=100 Ω, RCAL=R3=100 Ω |
| `CAL_HIGH_1K` | `0x11` | RFB=R9=100 Ω, RCAL=R4=1 kΩ |
| `CAL_LOW_1K` | `0x12` | RFB=R8=1 kΩ, RCAL=R4=1 kΩ |
| `CAL_LOW_10K` | `0x22` | RFB=R8=1 kΩ, RCAL=R7=10 kΩ |
| `RTD` | `0x41` | RFB=R9=100 Ω, DUT=RTD |
| `MEASURE_HIGH` | `0x81` | RFB=R9=100 Ω, DUT=celda |
| `MEASURE_LOW` | `0x82` | RFB=R8=1 kΩ, DUT=celda |
| `ALL_OFF` | `0x00` | Ocho interruptores abiertos |

Las siete rutas CN0349 proceden de la [guía de software de Analog Devices](https://wiki.analog.com/resources/eval/user-guides/circuits-from-the-lab/cn0349).
También documenta calibración alta con 100 Ω: `0x09`. `ALL_OFF` concuerda con la
semántica del ADG715 (cero = todos abiertos) y solo se aplica con el AD5934
detenido.

## Seguridad y lectura

Antes de cualquier escritura, `set`/`write` leen el registro de control AD5934.
Si falla la lectura, el modo no es A/B o Low no es `0x08`, no se transmite el
byte al ADG715. Esto presupone un solo hilo y un maestro I²C; futuras tareas
concurrentes requerirán arbitraje.

El cambio se envía como una palabra completa en una transacción. No se inserta
ALL_OFF automáticamente: el firmware no afirma que esa transición intermedia
sea necesaria o libre de transitorios. Solo se aplican las máscaras indicadas.
ADG715 readback confirma su registro de control, no continuidad, ruta analógica
ni resultado de medida.

`read` informa la lectura actual y S1…S8. `status` añade el preset coincidente,
último error retenido, última solicitud y último valor confirmado por readback.
La lectura real nunca se sustituye por valores guardados; ACK tampoco se toma
como prueba del estado físico.

El driver genérico ofrece `setSwitch/getSwitch` para canales 1–8; MuxApp bloquea
el comando individual para evitar combinaciones fuera de la tabla autorizada.
Consultar [el plan de pruebas de Hito 1C](../../test/hito1c.md), incluido el
registro del estado original y su restauración si es una máscara autorizada.

`selectPreset(name)` es la interfaz de aplicación usada por ImpedanceApp: lee el
estado actual, evita una escritura redundante si ya coincide y, si hace falta,
aplica el mismo interlock y readback que el comando de terminal. Durante una
adquisición, el indicador ocupado de ImpedanceApp bloquea cambios externos desde
el terminal.

Para el perfil de adquisición, MuxApp también expone `CalibrationPreset`,
`CalibrationInfo`, `resolveCalibrationPreset`, `calibrationPresetByName` y
`applyCalibrationPreset`. Esta tabla pública restringida contiene únicamente
`CAL_HIGH_100R` (`0x09`, 100 Ω/100 Ω), `CAL_HIGH_1K` (`0x11`, 100 Ω/1 kΩ),
`CAL_LOW_1K` (`0x12`, RFB/RCAL 1 kΩ/1 kΩ) y `CAL_LOW_10K` (`0x22`, 1 kΩ/10 kΩ).
Comparte sus nombres/máscaras con la lista
general ADG715 sin duplicar la metadata de resistencias en ImpedanceApp. El
driver AD5934 genérico no depende de ADG715.

En Hito 1E MuxApp también expone `ping`, `readMask`, `address`, error técnico y consulta de máscara conocida a ImpedanceApp. La recuperación deliberadamente no conmuta el multiplexor ni cambia el preset: lee y conserva la máscara existente. El diagnóstico integrado solo identifica si la máscara coincide con un estado autorizado; no mide la ruta eléctrica. Consultar [`test/hito1e.md`](../../test/hito1e.md).

En 2A, MuxApp conserva la última máscara obtenida por lectura o escritura
verificada. CN0349Manager usa ese dato cacheado para distinguir preset solicitado
del aplicado, sin I²C oculto en `getStatus()`.

## Hito 2C.1

`pt100Info()` y `applyPt100()` reutilizan el preset histórico RTD/0x41 y su
interlock/readback. RTD sigue sin ser un preset RCAL; no cambia la enumeración de
las cuatro calibraciones 2B. La API de medida del Manager solo solicita esta ruta,
sin añadir selección de conductividad. [Revisión del circuito](../../docs/pt100-route.md).
