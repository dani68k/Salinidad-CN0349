# Driver AD5934 — Hitos 1B y 1D.1

`AD5934` proporciona protocolo de registros y operaciones acotadas para el
controlador. Usa una referencia `TwoWire&`, dirección configurable (por defecto
`0x0D`) y no inicializa el bus ni imprime. No depende del terminal, ADG715 ni
CN0349 Manager. Las operaciones secuenciales de una adquisición son coordinadas
por `ImpedanceApp`.

Referencias: [datasheet AD5934 Rev. E](https://www.analog.com/media/en/technical-documentation/data-sheets/ad5934.pdf),
tablas 7–13 y figuras 24, 26–30. Para el reloj y las rutas de resistencias, la
[nota CN-0349 Rev. A](https://www.analog.com/media/en/reference-design-documentation/reference-designs/cn0349.pdf).

## Reloj y perfil fijo

El AD5934 requiere una señal MCLK externa; no ofrece un selector de reloj interno
en este mapa de control. CN-0349 monta un oscilador externo de 1 MHz. El perfil
1D.1 usa ese MCLK, 2 kHz, frecuencia DDS `0x041893`, Range 1 (2 Vpp nominal),
PGA ×1, incremento 0, número de incrementos 0 y settling 10 ciclos ×1. El límite
de frecuencia configurado por la aplicación es MCLK/16 (62.5 kHz con este MCLK).

La palabra DDS se calcula como `floor(frequency * 2^27 / MCLK)`. La frecuencia
real resultante está cuantizada por esa palabra. El firmware no puede comprobar
por I²C que el oscilador esté físicamente presente; ACK tampoco prueba que el
MCLK funcione.

## Acceso y protección

Lecturas se hacen byte a byte: seleccionar puntero `0xB0, registro`, STOP y luego
Receive Byte. Escrituras de configuración usan dirección y dato explícitos. No
se usa Block Write en los registros de control. Campos reservados se validan,
Control Low conserva D3=1, y las operaciones de control nunca escriben D4 reset.

`enterStandby()` limita la transición segura y verifica ambos bytes. Las APIs de
configuración cubren palabra start, incrementos, número de incrementos, settling,
Range 1/PGA ×1 y comandos Initialize/Start Sweep/Increment Frequency. Las
operaciones preservan D11:D8 al cambiar D15:D12; no hay comando para reset.
Antes de Increment Frequency, el driver exige D1 válido y Control Low `0x08`;
los bits de operación retenidos no se usan como estado de secuencia. ImpedanceApp
solo lo llama tras leer y almacenar REAL/IMAG del punto actual.
`wakeToStandby()` realiza únicamente la transición power-down→standby conservando
rango/PGA y verifica el control; la app la ejecuta tras fijar Range/PGA y antes de
programar los parámetros de barrido, conforme a la Figura 24.
`readReal()` / `readImaginary()` decodifican 16 bits con signo, MSB primero.
`AD5934Math` contiene funciones puras de frecuencia DDS, validación, decodificación,
magnitud, fase y límites de barrido. Magnitud/fase son del vector DFT crudo, no
ohmios ni fase calibrada.

La aplicación verifica registros después de configurarlos y solo lee Real/Imag
si Status D1 (máscara `0x02`) indica datos válidos. D2 no sustituye D1. El cálculo
de DFT tarda un tiempo finito; polling y timeout los gestiona la aplicación con
`millis()`, sin bucles de espera ni `delay()`.

## Límites

Una escritura multibyte se realiza byte a byte y puede quedar parcial ante fallo;
los errores conservan fase, registro y cantidad completada. No hay reintentos
automáticos. El bus usa el timeout de 25 ms de `main`; las operaciones de Wire son
síncronas y una transacción puede bloquear hasta ese timeout. No hay exclusión
frente a otro maestro o tareas concurrentes.

El driver no calcula impedancia, conductividad, salinidad ni factores de
calibración. No valida la amplitud real, fase eléctrica, exactitud del reloj o
repetibilidad analógica. Los procedimientos de punto único y barrido están en
[`test/hito1d1.md`](../../test/hito1d1.md) y [`test/hito1d2.md`](../../test/hito1d2.md).

## Diagnóstico integrado (Hito 1E)

Los códigos tipados de error pertenecen a `ImpedanceApp`; el driver sigue exponiendo su error técnico de transacción (`ErrorCode`, fase, registro, Wire y bytes completados). El nivel superior mapea NACK, timeout, lectura corta y bus al historial persistente en RAM. `enterStandby()` ofrece la primitiva de recuperación verificada; el driver no reinicia dispositivos ni borra historial. El comando `ad5934 diag` lee Status/configuración sin iniciar conversión. Plan: [`test/hito1e.md`](../../test/hito1e.md).
