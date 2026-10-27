# Hito 2E.1 — ruta EC y límites del modelo eléctrico

Fecha: 2026-10-27. Este documento describe el uso de resistencias externas en
J1 Yx-1/Yx-2; no se calcula conductividad ni se interpreta una celda real.

## Evidencia y topología

La [nota oficial CN-0349 Rev. A, figura 2 y tabla 1](https://www.analog.com/media/en/reference-design-documentation/reference-designs/cn0349.pdf)
describe una cadena de excitación AD5934 VOUT, acoplamiento C1 y buffer AD8606
U2A, entrada externa YCELL, recepción AD8606 U2B y AD5934 VIN. La tabla 1
establece `0x81`/RFB R9=100 ohm para medida HIGH y `0x82`/RFB R8=1 kohm
para LOW. Las calibraciones correspondientes son `0x09`/`0x11` y
`0x12`/`0x22`. S8 está activo en ambas máscaras de medida; S1 selecciona R9
en HIGH y S2 selecciona R8 en LOW. Los dos terminales de celda corresponden a
[J1 pines 4 y 5 según la guía oficial](https://wiki.analog.com/resources/eval/user-guides/circuits-from-the-lab/cn0349).
Confirmar serigrafía/orientación en la placa antes de conectar resistencias.

La ruta EC del CN-0349 incorpora R2=100 ohm en serie con la carga externa,
como indica la nota para ampliar el extremo de alta conductancia. La figura publicada
es un esquema **simplificado**; no aparece en este repositorio el netlist ni el
esquema de fabricación de la revisión física concreta. No se ha verificado la
trayectoria exacta de R2 en ambas configuraciones, las derivaciones o elementos
parásitos, ni la transferencia completa entre J1, AD8606 y DFT. El readback
`0x81`/`0x82` comprueba un registro digital, no la continuidad analógica.
Se solicita contrastar el esquema de fabricación o medir continuidad con la
placa sin alimentación antes de atribuir una diferencia numérica a R2.

La resistencia externa, R2 y el resto de la cadena EC pueden afectar
`Z_aparente`. **No** se impone
`Z_aparente=Rexterna+100 ohm`. El firmware conserva la medida bruta y no resta
100 ohm ni un offset medido. La relación de fase es instrumental, no fase
física validada de una celda. C1 y las etapas AD8606 también condicionan la
transferencia AC; el GF de una ruta RCAL puede no caracterizar por completo la
ruta externa. Además, la nota oficial describe calibración de admitancia con
dos puntos y offset, mientras este firmware 2B usa un GF de un punto por
slot. No se transplanta la afirmación de exactitud del software oficial al
método presente.

## Flujo implementado

`CN0349Manager` selecciona HIGH/LOW y una calibración con el mismo RFB. Valida
slot, frecuencia DDS efectiva, MCLK externo, excitación, PGA y settling. El
motor `ImpedanceApp` conmuta y verifica el ADG715, configura AD5934 a 2000 Hz
y adquiere REAL/IMAG de modo asíncrono, terminando en standby. Se usa
`M=sqrt(REAL²+IMAG²)`, `Z_aparente=1/(GF*M)` y fase instrumental normalizada
a [-180°,180°]. No hay autorange, constante de celda, conductividad ni
compensación térmica. La medición EC conserva snapshot propio de máscara, ruta,
RFB, GF/ID y DFT; RTD/temperatura conservan snapshots independientes. El bit
`saturationKnown=false` expresa que DFT y status no permiten certificar ausencia
de saturación; magnitud cero y valores numéricos inválidos se rechazan.

La arquitectura completa está en [architecture.md](architecture.md) y la
validación física planificada en [hito2e1.md](../test/hito2e1.md).
