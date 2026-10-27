# Hito 2C.1 — Ruta Pt100 y modelo inicial

Fecha: 2026-10-27. Medida aparente exclusivamente a 2000 Hz.

## Evidencia de circuito y conexión

No se encontró esquema PDF/Altium en el repositorio inspeccionado. Se consultaron
la [nota original CN-0349 Rev. A, figura 1 y tabla 1](https://www.analog.com/media/en/reference-design-documentation/reference-designs/cn0349.pdf)
y la [guía oficial, Connecting the Hardware](https://wiki.analog.com/resources/eval/user-guides/circuits-from-the-lab/CN0349).
El [paquete Design Support](https://www.analog.com/media/en/reference-design-documentation/design-integration-files/CN0349-DesignSupport.zip)
redirigió a una descarga protegida y no se pudo revisar el esquema de fabricación.
No se presenta esta revisión como verificación completa del netlist ni del pinout físico.

La guía identifica Pt100 en J1 **pines 2 y 3**, celda en 4 y 5, blindaje en 1.
Antes de conectar, identificar físicamente PT100-1/PT100-2, orientación y pin 1
mediante serigrafía y continuidad con placa apagada. No asignar izquierda/derecha
ni asegurar cuál de 2/3 corresponde a cada etiqueta sin el esquema completo.

La tabla 1 identifica `0x41` como RTD con RFB=R9=100 Ω. La máscara cierra S1 y S7.
R3=100 Ω pertenece a RCAL (`0x09`); R9/S1 es realimentación. U2A/AD8606
acopla/buferea la excitación y U2B convierte corriente en tensión hacia VIN del
AD5934. C1 es acoplamiento; R1/R5 y la referencia VDD/2 son polarización.
Estos elementos no deben confundirse con una resistencia en serie con el DUT.

## Equivalente de trabajo e incertidumbre

```text
DDS/VOUT AD5934 -> acoplamiento/buffer AD8606 -> DUT externo en J1
    -> selección Pt100 S7 -> etapa corriente/tensión AD8606 -> VIN AD5934 -> DFT
                            ^ realimentación R9 + S1 (100 ohm nominal)
```

Es un equivalente funcional inicial, no una reconstrucción certificada de todas
las redes. La distribución exacta de componentes, referencias y conexiones hasta
RFB/VIN debe contrastarse con el esquema de fabricación. R3 no justifica restar
100 Ω. La resistencia de switches, contactos, cables, salida del buffer,
acoplamiento y diferencias entre ruta RCAL y ruta externa pueden alterar la
transferencia. No se cuantifican ni se restan. Tampoco se traslada a Pt100 una
corrección propia de la ruta de conductividad (R2).

## Fórmulas y alcance

- `M = sqrt(REAL² + IMAG²)`, código DFT; REAL/IMAG con signo de 16 bits.
- `GF = 1 / (RCAL * Mcal_media)`, unidades `1/(Ω * código DFT)`; ya disponible en 2B.
- `Z_aparente = 1 / (GF * Mmedida)`, Ω, escalar positivo.
- `fase_raw = atan2(IMAG, REAL)`, grados.
- `fase_corregida = normalizar(fase_raw - fase_calibracion)` en [-180°, +180°].

Hipótesis: cadena lineal estable e inversamente proporcional a impedancia,
calibración electrónica transferible con la misma configuración. Son hipótesis
para comprobar en banco. La fase es **corregida instrumentalmente**, no fase
física validada. `valid` confirma adquisición, final seguro y cálculo numérico;
no demuestra precisión absoluta ni ausencia de saturación/señal insuficiente.
No se implementan umbrales de saturación inventados ni incertidumbre ficticia.

Calibraciones manuales: `CAL_HIGH_100R` (`0x09`, RCAL 100 Ω) y `CAL_HIGH_1K`
(`0x11`, RCAL 1000 Ω), ambas RFB 100 Ω. Las rutas LOW se rechazan. Se comprueban
slot válido, GF finito positivo, frecuencia solicitada, DDS efectiva, MCLK,
reloj externo, excitación, PGA y settling. La comparación de frecuencia efectiva
admite únicamente el redondeo de las cinco sumas float de 2B
(`5 * FLT_EPSILON * max(f1,f2)`), no una tolerancia física.

No hay promedio entre GF, autorango, calibración con la carga esperada,
corrección serie, resistencia externa corregida, impedancia compleja,
conductividad, temperatura, persistencia, BLE ni webapp. La API conserva espacio
conceptual para futuros modelos sin introducir campos metrológicos ficticios.
Ver [procedimiento de ensayo](../test/hito2c1.md).
