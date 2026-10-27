# ImpedanceApp — diagnóstico y adquisición AD5934

Actualización 2026-10-27: `requestPt100(busReady, quiet=false)` permite omitir
mensajes informativos en una adquisición continua. La opción se guarda por
operación y se restablece en cada inicio. Suprime el aviso de inicio y el informe
RAW/standby correcto, y se propaga a la selección RTD de MuxApp. Los errores,
comprobaciones de hardware, estados y datos adquiridos se conservan. Las
adquisiciones manuales siguen siendo verbosas por defecto.

Hito 2E.1, 2026-10-27: `requestEc(busReady, mask)` acepta únicamente las rutas
EC definidas por MuxApp (`0x81` y `0x82`). `startMeasurement` recibe la máscara
de ruta para escoger RTD, EC o preset de calibración; todas usan el mismo estado
DFT, readback, errores y standby. `Result::calibration` conserva la ruta
efectivamente aplicada para la comprobación posterior del Manager. No hay
duplicación del driver, offsets ni cálculo de conductividad. Errores de ruta
siguen el flujo de recuperación existente.

`ImpedanceApp` enlaza comandos serie, driver AD5934 y `MuxApp`. Coordina la
selección configurable del preset de calibración permitido, programación de
perfil, adquisición DFT y recuperación a standby. El driver sigue siendo
independiente del multiplexor. La máquina de
estados avanza una operación acotada por `update()` y el terminal admite status,
result y stop mientras está ocupada.

## Comandos

```text
ad5934 measure help
ad5934 measure config
ad5934 measure start
ad5934 measure status
ad5934 measure result
ad5934 measure stop
ad5934 measure set freq <hz>
ad5934 measure set settling <cycles>
ad5934 measure set preset <NAME>
ad5934 measure sweep help
ad5934 measure sweep config
ad5934 measure sweep set start <Hz>
ad5934 measure sweep set step <Hz>
ad5934 measure sweep set increments <n>
ad5934 measure sweep start
ad5934 measure sweep status
ad5934 measure sweep result
ad5934 measure sweep stop
```

Los únicos presets de adquisición son `CAL_HIGH_100R`, `CAL_HIGH_1K`,
`CAL_LOW_1K` y `CAL_LOW_10K`. La selección es lógica y no conmuta el ADG715
hasta `start`. Presets de medida, RTD, ALL_OFF y máscaras arbitrarias no son
aceptados por este ajuste.

El perfil inicial usa MCLK externo 1 MHz (oscilador de la placa CN-0349), 2 kHz,
Range 1 / 2 Vpp nominal, PGA ×1, preset `CAL_LOW_1K` (0x12), cero incrementos y
settling 10 ciclos. `set` cambia el perfil RAM mientras no haya operación activa;
frecuencia permitida 1..MCLK/16, settling 0..511. El preset seleccionado también
se mantiene en RAM mientras el equipo está encendido; tras reiniciar el valor por
defecto es CAL_LOW_1K. No altera la frecuencia MCLK.

## Secuencia

`start` comprueba que el bus y AD5934 estén disponibles e inactivos con Control
Low=`0x08`; resuelve el identificador de preset en MuxApp y selecciona/verifica
la máscara del ADG715. La selección manual previa del multiplexor no cambia el
preset lógico configurado: `start` aplicará este último. Un fallo de selección
cancela la adquisición antes de la DFT e intenta mantener/verificar standby.
Programa Range/PGA en power-down, pasa
explícitamente a standby y verifica esa transición, y después programa los
registros de frecuencia, incrementos y settling, verificando los campos byte a
byte. Luego envía Initialize with Start Frequency, espera el intervalo configurado, envía
Start Frequency Sweep y sondea Status cada 5 ms. Solo D1 autoriza leer REAL/IMAG.
Tras leer ambos, calcula magnitud cruda en doble precisión y pasa el dispositivo
a standby con readback.

Un timeout de 500 ms desde el comando Start Sweep, fallo I²C o D1 no observado
marca error e intenta recuperar standby. No se repite la adquisición. Stop también
inicia recuperación. El estado anterior del ADG715 y los registros de perfil no
se restauran; el multiplexor queda en el preset de adquisición seleccionado y el
AD5934 en standby al acabar. Cada resultado guarda su propio identificador y los
valores nominales RFB/RCAL, aunque se cambie la configuración antes de consultar
un resultado anterior. No se calcula impedancia ni hay conversión de fase.

## Barrido 1D.2

El barrido reutiliza los estados de configuración, standby, Initialize, espera
D1, lectura de REAL/IMAG y recuperación de la adquisición única. El estado
`INCREMENTING` solo se ejecuta después de almacenar el punto actual. Cada
increment reinicia el timeout de 500 ms; no se reenvía Initialize entre puntos.
Al leer el último punto se consulta D2 una sola vez; según AD5934 Rev. E, D2 se
activa al completar el procesamiento del último punto. Si falta D2 se informa
error y se recupera standby, sin enviar un incremento extra ni esperar sin límite.

Los resultados ocupan un arreglo fijo de 64 puntos. Se conserva el prefijo válido
ante error/cancelación y `result` lo etiqueta parcial; nueva adquisición única no
borra el último barrido. Un nuevo barrido reemplaza el conjunto previo. Frecuencia
mostrada por punto se deriva del código DDS acumulado, evitando acumular error en
Hz enteros. Fase se calcula con atan2(IMAG, REAL), en grados, sin corregir la fase
del sistema. El conjunto guarda su configuración y preset usados. No hay NVS ni
cálculo de impedancia.

La validación física se describe en [`test/hito1d2.md`](../../test/hito1d2.md).

## Hito 1E — estado, errores y recuperación

`ImpedanceStatus` separa comunicación AD5934/ADG715, configuración verificada,
estado operativo, diagnóstico completo y `hardwareReady`. Un ACK no establece
readiness. `ad5934 diag` hace lecturas no destructivas de control/configuración,
Status y máscara ADG715; la máscara debe corresponder a un preset conocido para
declarar estado operativo. `ad5934 recover` sondea ambos dispositivos, lleva
AD5934 a standby verificado y conserva sin cambio la máscara actual del ADG715.
Una adquisición requiere readiness demostrado por diagnóstico/recovery.

Los errores se guardan como código, subsistema, fase de aplicación, millis,
detalle, recuperabilidad y estado de recuperación. Se conserva por separado el
último error, el activo y contadores acumulados. `errors clear` se rechaza durante
busy o mientras quede un error activo. Lecturas correctas no borran historial.
`measure sweep stop` preserva puntos previos y libera busy al terminar la
verificación de standby.

Comandos: `ad5934 status`, `diag`, `errors`, `errors clear` y `recover`.
Comportamiento y plan físico: [`test/hito1e.md`](../../test/hito1e.md). Las
transacciones Wire siguen siendo síncronas y limitadas por timeout I2C (25 ms);
diag/recover pueden hacer varias en un handler. Fases de control/configuración
expiran a 1500 ms y DFT a 500 ms. Esta versión no incorpora mocks de fallos ni
validación física 1E.

En Hito 2A, `CN0349Manager` usa esta instancia como único motor. `configure*`,
`request*`, cancelación, resultados y estado son wrappers sobre las estructuras y
máquina ya existentes. La terminal heredada sigue conectada aquí; los comandos
`cn0349` se despachan al Manager. Ver [`src/cn0349/README.md`](../cn0349/README.md).

La secuencia sigue la [Figura 24 del datasheet Rev. E](https://www.analog.com/media/en/technical-documentation/data-sheets/ad5934.pdf).
MCLK 1 MHz y la frecuencia 2 kHz corresponden al oscilador del circuito
[CN-0349 Rev. A](https://www.analog.com/media/en/reference-design-documentation/reference-designs/cn0349.pdf).
Las pruebas 1D.1 están en [`test/hito1d1.md`](../../test/hito1d1.md); los cuatro
presets configurables y los dos puntos por rango se validan según
[`test/hito1d1b.md`](../../test/hito1d1b.md).

## Hito 2C.1

`requestPt100(busReady)` entra en la misma máquina single mediante un argumento
explícito de ruta. MuxApp resuelve y aplica RTD con readback; no se introduce
estado persistente de ruta que pueda afectar a single/sweep/calibraciones
posteriores. El resultado RAW conserva metadatos RTD (ID de calibración no
aplicable, 0xFF). `pt100Info()` expone esos metadatos al Manager.
Se reinicia `cancelled_` al iniciar una adquisición nueva: una cancelación anterior
no debe cancelar conversiones posteriores. El test host reproduce esa regresión.
El final solo publica DFT válido tras confirmar standby. La conversión a ohmios
pertenece al Manager; ver [modelo Pt100](../../docs/pt100-route.md).
