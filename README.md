# ATLANTES CN-0349

Firmware Arduino/PlatformIO para ESP32-S3 y la placa original EVAL-CN0349-PMDZ.
Este archivo es el diario acumulativo: añadir nuevas entradas sin sustituir el
historial de los hitos anteriores.

## Estado actual — Hito 2A, firmware 0.7.0

CN0349Manager expone API C++ y comandos `cn0349` encima de la única máquina
ImpedanceApp. Compilación PlatformIO ESP32-S3 correcta; validación física 0.7.0
y pruebas host de integración están pendientes.

## Hito 1E — validación física previa (firmware 0.6.0)

Hito 1E añade estado integrado, errores tipados, diagnóstico no destructivo y
recuperación controlada. Compilación PlatformIO ESP32-S3 correcta; pruebas host
no ejecutables aquí por ausencia de gcc/g++. Validación física inicial reportada:
diag/status READY, barrido 9/9 y recuperación a standby; siguen pendientes las
pruebas de fallos, cancelación y repetición completa.

## Historial inmediato — Hito 1D.2

La aplicación implementa un punto DFT del AD5934 con el perfil CN-0349 de MCLK
externo 1 MHz y excitación 2 kHz. La versión 0.4.0 venció esperando Status D1.
La versión 0.4.1 corrige la secuencia de standby requerida antes de Initialize;
compiló correctamente y completó una primera adquisición física con D1 válido.
0.5.1 corrigió una primera precondición de Increment Frequency, pero el usuario
reportó que todavía fallaba con validación en `0x80`. 0.5.2 elimina la dependencia
de los bits de operación retenidos: mantiene D1 y Control Low `0x08` como
precondiciones. Compila para ESP32-S3 (`SUCCESS`, 7,89 s; RAM 25 264 bytes,
Flash 376 918 bytes); las pruebas host quedan pendientes en un entorno con
compilador C/C++ host. El usuario confirmó en hardware el barrido predeterminado:
9/9 puntos válidos, de 999,995 a 2 999,969 Hz, con reporte final de standby.
Esto valida el flujo digital de barrido; los valores son DFT cruda y no verifican
impedancia ni exactitud analógica.

```text
ad5934 help
ad5934 ping
ad5934 standby
ad5934 read <reg>
ad5934 read16 <reg>
ad5934 read24 <reg>
ad5934 write <reg> <value>
ad5934 write16 <reg> <value>
ad5934 write24 <reg> <value>
ad5934 status
ad5934 dump
adg715 help
adg715 ping
adg715 read
adg715 status
adg715 presets
adg715 set <preset>
adg715 write <mask>
adg715 off
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

Registros y valores en hexadecimal (`0x` opcional). Empezar por `ad5934 ping`,
`ad5934 read 0x8F`, `ad5934 status`, `ad5934 read16 0x88`,
`ad5934 read24 0x82` y `ad5934 dump`. No asumir valores iniciales de configuración.
Las escrituras están limitadas a campos de configuración y requieren modo
power-down/standby. `ad5934 standby` permite una transición verificada y limitada
si el chip está activo. Seguir [test/hito1b.md](test/hito1b.md) para pruebas con
restauración del valor original. `ad5934 measure start` es el único comando
que inicia ahora una conversión DFT; los comandos de registros siguen sin iniciar
DDS ni hacer mediciones.

## Uso del Hito 1A

Entorno: `esp32-s3-devkitc-1-n32r8v`. Se conserva la configuración base con
`platform = espressif32`, `framework = arduino` y monitor a 115200 baudios.
El monitor tiene eco local activado y `monitor_filters = direct` para transmitir
las secuencias ANSI de `clear` al terminal sin convertirlas en texto visible.

```sh
pio run -e esp32-s3-devkitc-1-n32r8v
pio run -e esp32-s3-devkitc-1-n32r8v -t upload
pio device monitor -b 115200
```

Ejecutar desde una terminal con PlatformIO en PATH, o utilizar las tareas Build,
Upload y Monitor de la extensión. En este equipo también se puede ejecutar en
PowerShell `& "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe" run -e esp32-s3-devkitc-1-n32r8v`.
Seleccionar el puerto real si hay varios conectados (`--upload-port COMx` al
cargar; `-p COMx` para el monitor). La carga modifica el firmware de la placa.

El bus usa SDA GPIO4, SCL GPIO5 y 100000 Hz. Conectar masa común y comprobar
alimentación, conexiones y pull-ups apropiados para la placa antes de probar.
La configuración conservada no añade USB CDC: utilizar la conexión que exponga
`Serial` con la definición de placa instalada (UART0/puente USB-UART en la
configuración revisada). El firmware no espera al monitor; si se pierde la
cabecera, abrir el monitor y reiniciar la placa, o ejecutar `info`.

| Comando | Resultado |
| --- | --- |
| `help` | Lista de comandos y descripciones. |
| `info` | Nombre, versión, hito, GPIO, frecuencia y timeout configurados. |
| `i2c scan` | Sondeo de 112 direcciones, de `0x08` a `0x77`, y resumen. |
| `i2c status` | Inicialización, escaneo en curso y último diagnóstico completo. |
| `clear` | Secuencia ANSI para limpiar pantalla y situar el cursor al inicio. |

Se aceptan mayúsculas/minúsculas, espacios y tabuladores entre palabras, y finales
CR, LF o CRLF. El máximo es de 95 caracteres recibidos por línea. Una línea que
supera el límite se descarta hasta su terminación; no se ejecuta parcialmente.
Las líneas vacías se ignoran. El firmware no hace eco; lo proporciona el monitor
mediante `monitor_echo = true`. ANSI depende de la compatibilidad del terminal.
Si `clear` muestra `␛[2J␛[H`, cerrar el monitor con Ctrl+C y volver a abrirlo para
aplicar `monitor_filters = direct`. No requiere recompilar ni cargar firmware.
También puede probarse con `pio device monitor -b 115200 -f direct` en el terminal
integrado de VS Code. Se limpia la pantalla visible; el historial de desplazamiento
puede seguir disponible.

El escaneo muestra exclusivamente las direcciones que dan ACK, sin identificar
los IC. La dirección `0x0D` del AD5934 indicada en los requisitos no se usa para
suponer presencia ni identidad. No hay número esperado de dispositivos.

La arquitectura está en [docs/architecture.md](docs/architecture.md) y el
procedimiento de aceptación en [test/README.md](test/README.md).

## 2026-10-26 — Hito 1A: infraestructura I²C y terminal

**Objetivo:** establecer el arranque, configuración I²C y terminal de diagnóstico
sin drivers completos ni lógica de medida.

**Archivos modificados:**

- `src/main.cpp`: reemplaza la plantilla por la inicialización y el ciclo cooperativo.

**Archivos creados:**

- `include/config.h`: identidad, pines, frecuencias, timeout y límites del terminal.
- `src/terminal/Terminal.h` y `src/terminal/Terminal.cpp`: terminal y escaneo incremental.
- `README.md`: uso y diario acumulativo.
- `docs/architecture.md`: responsabilidades y evolución prevista.
- `include/README.md`, `src/README.md`, `src/terminal/README.md`: interfaces implementadas.
- `lib/README.md`, `lib/AD5934/README.md`, `lib/ADG715/README.md`: separación y documentación de drivers.
- `src/impedance/README.md`, `src/mux/README.md`, `src/cn0349/README.md`: módulos previstos.
- `test/README.md`: comprobaciones de compilación y aceptación física.

Se mantienen los archivos `README` originales de la plantilla y `platformio.ini`.

**Decisiones de arquitectura:** `main.cpp` es propietario de la inicialización de
`Wire` y `Serial`. `Terminal` recibe referencias a `Stream` y `TwoWire`, no depende
de drivers y concentra los comandos en una tabla estática. Configuración en un
único encabezado. No se añaden dependencias, `String` ni asignaciones dinámicas
en el código de aplicación; las bibliotecas Arduino gestionan sus propios buffers.

**Funcionalidades:** cabecera de arranque; cinco comandos; normalización de entrada;
CRLF sin doble ejecución; límites y recuperación tras errores; escaneo sin bytes
de datos ni accesos a registros; distinción entre ACK, NACK de dirección y errores;
retención del último escaneo completo y del último error de ese escaneo.

Cada llamada a `update()` consume como máximo 32 bytes y sondea como máximo una
dirección. `Wire` es síncrono y tiene timeout configurado de 25 ms por transacción;
la salida serie también puede introducir espera. El diseño es cooperativo, sin
bucles de espera, `delay()` ni espera de conexión serie; no promete tiempo real
estricto. Al añadir tareas concurrentes habrá que arbitrar el acceso a `Wire`.

**Pruebas y resultados:** `pio run -e esp32-s3-devkitc-1-n32r8v` finalizó con
`SUCCESS` (21,89 s), generando `firmware.elf` y `firmware.bin`. RAM: 21.840 bytes
de 327.680 (6,7 %); flash de aplicación: 346.898 bytes de 13.107.200 (2,6 %).
Entorno instalado: Espressif32 54.3.20, Arduino-ESP32 3.2.0, bibliotecas ESP-IDF
5.4.0 y toolchain Xtensa 14.2.0. El primer intento quedó limitado por permisos
del archivo `platforms.lock` global; la ejecución con acceso autorizado completó
la compilación. Se revisó el código contra la API `Wire` instalada, incluidos
sus códigos de retorno. No se han ejecutado pruebas funcionales automatizadas ni
pruebas físicas, ni se ha cargado el firmware en una placa. El procedimiento
manual queda documentado.

**Limitaciones y pendientes:** comprobar arranque, terminal y respuestas I²C en
el hardware real. Un ACK no identifica el dispositivo ni acredita su funcionamiento
como sensor. Cero ACK sin otros errores es un resultado posible. La frecuencia
mostrada es la configurada, no una medición eléctrica. La inicialización correcta
de `Wire` no valida cableado ni dispositivos. El entorno `espressif32` se conserva
sin fijar versión, por lo que otra instalación puede resolver paquetes distintos.
Drivers AD5934/ADG715, adquisición, multiplexación funcional, CN0349 Manager,
BLE y web quedan pendientes de hitos posteriores.

**Estado:** preparado para validación física: implementación, documentación y
compilación terminadas correctamente; aceptación en hardware pendiente.
En ese punto no se había iniciado el Hito 1B.

## 2026-10-26 — Hito 1A: validación parcial y ajuste del monitor

**Objetivo:** registrar las salidas de hardware aportadas por el usuario y resolver
la visualización literal de las secuencias ANSI de `clear` en PlatformIO.

**Archivos modificados:** `platformio.ini`, `README.md` y `test/README.md`.

**Cambio y decisión:** se añade `monitor_filters = direct`, conservando
`monitor_echo = true`, que ya estaba presente, y la configuración de placa,
framework y velocidad. El firmware ya emite `ESC[2J` y `ESC[H`; no necesita cambios.
El filtro `direct` deja pasar ANSI sin transformación, según la
[documentación del monitor de PlatformIO](https://docs.platformio.org/en/latest/core/userguide/device/cmd_monitor.html#filters).
Reabrir el monitor aplica el ajuste; no es necesaria una nueva carga.

**Pruebas físicas reportadas por el usuario:** `help` muestra los cinco comandos;
`info` muestra ATLANTES CN-0349, 0.1.0, 1A, GPIO4/5, 100000 Hz y timeout de 25 ms.
`i2c scan` informa ACK en `0x0D` y `0x48`, con 112 sondeos, 2 ACK, 110 NACK y
0 errores. `i2c status` conserva esos totales y muestra inicialización correcta.
Estas salidas no identifican por sí solas los dispositivos ni validan mediciones.

**Incidencia:** `clear` muestra literalmente `␛[2J␛[H` con el filtro anterior.
Se ajusta el monitor; la limpieza visual tras reabrirlo queda pendiente de
confirmación del usuario. La compilación del firmware sigue siendo la registrada
en la entrada anterior; este ajuste del monitor no modifica código compilado.

**Estado:** validación física parcial. Pendientes cabecera de arranque, entradas
desconocidas/largas, terminaciones CR/LF/CRLF y confirmación visual de `clear`.
El desarrollo permanece en el Hito 1A.

## 2026-10-26 — Resumen del Hito 1A para iniciar 1B

La infraestructura 1A implementa arranque sin espera del monitor, bus Wire en
GPIO4/5 a 100 kHz, timeout de 25 ms, terminal con 95 caracteres útiles y soporte
CR/LF/CRLF; incluye `help`, `info`, `i2c scan`, `i2c status` y `clear`.

Pruebas físicas reportadas por el usuario: `help`, `info`, escaneo con ACK en
`0x0D` y `0x48` (112 direcciones, 110 NACK, 0 errores), `i2c status` con historial
correcto y `clear` funcionando tras ajustar el monitor. Un ACK no identifica por
sí solo un IC. El usuario declara 1A validado como base; la cabecera, los límites
de entrada y las terminaciones de línea se conservan en el plan de regresión porque
no se aportaron sus salidas individuales.

## 2026-10-26 — Hito 1B: driver AD5934 y diagnóstico de registros

**Objetivo:** acceso reutilizable a registros documentados y diagnóstico desde el
terminal, antes de implementar medidas.

**Cambios:** driver AD5934 independiente del terminal con dirección configurable,
lectura/escritura de 8/16/24 bits, mapa, permisos y errores de Wire; aplicación
ImpedanceApp con validación de argumentos, ping, estado, lecturas, escrituras
verificadas y dump incremental. Terminal integrado por callback. Firmware
identificado como 0.2.0 / 1B.

**Decisiones:** protocolo Address Pointer `0xB0` y Receive Byte para lectura, Write
Byte para escritura, dirección explícita byte a byte y orden MSB primero. Control
bloqueado por seguridad; el terminal solo permite configuración con modo inactivo,
y relectura de verificación. Se rechazan registros no documentados, spans que
atraviesan grupos y bits no permitidos. Dumps y escaneos no se solapan. Referencia:
[datasheet AD5934 Rev. E](https://www.analog.com/media/en/technical-documentation/data-sheets/ad5934.pdf).

**Archivos nuevos:** `lib/AD5934/AD5934.h`, `lib/AD5934/AD5934.cpp`,
`src/impedance/ImpedanceApp.h`, `src/impedance/ImpedanceApp.cpp`, `test/hito1b.md`.

**Archivos modificados:** `include/config.h`, `src/main.cpp`,
`src/terminal/Terminal.h`, `src/terminal/Terminal.cpp`, `README.md`,
`docs/architecture.md`, `include/README.md`, `lib/README.md`,
`lib/AD5934/README.md`, `src/README.md`, `src/impedance/README.md`,
`src/terminal/README.md` y `test/README.md`.

**Pruebas:** `pio run -e esp32-s3-devkitc-1-n32r8v` completó con `SUCCESS` en
20,12 s; RAM 21.888/327.680 bytes (6,7 %) y flash 353.762/13.107.200 bytes (2,7 %).
El mapa y el protocolo se revisaron frente a Rev. E y la API Wire instalada.
En el registro inicial del Hito 1B no se habían ejecutado pruebas físicas ni
funcionales automatizadas; el procedimiento está en `test/hito1b.md`.

**Limitaciones:** ACK no identifica el chip; `requestFrom()` comunica longitud,
no distingue el motivo de error de recepción. Una escritura interrumpida puede
dejar registros parcialmente actualizados; se informa el fallo sin rollback.
Las lecturas multibyte no son una instantánea atómica. La exclusión actual es de
un único hilo, no un mutex para BLE. Datos Real/Imaginary se entregan en bruto.

**Estado:** implementación y compilación terminadas; validación física 1B pendiente.
No hay generación DDS, barrido, DFT, calibración ni medición de conductividad;
en ese momento aún no se había iniciado Hito 1C ni el driver ADG715.

## 2026-10-26 — Hito 1C: driver ADG715 y rutas CN0349

Se añadió un driver ADG715 genérico con lectura de registro, escritura de byte
completo verificada por readback, consulta de interruptores S1–S8 y errores
detallados. `MuxApp` expone ping, lectura, estado y presets CN0349; únicamente
permite las máscaras documentadas y exige una lectura fresca del AD5934 en
power-down/standby con Control Low=`0x08` antes de cada cambio. No modifica el
AD5934, no genera una transición ALL_OFF automática y rechaza el control de
interruptores individuales desde la aplicación.

Los presets se contrastaron con la [guía CN0349 de Analog Devices](https://wiki.analog.com/resources/eval/user-guides/circuits-from-the-lab/cn0349).
El protocolo de byte y readback se contrastó con el [datasheet ADG714/ADG715 Rev. E](https://www.analog.com/media/en/technical-documentation/data-sheets/adg714_715.pdf).
El mapa incluye `CAL_HIGH_100R = 0x09` además de las siete rutas previstas.

Se amplió `Terminal` a dos módulos estáticos para conservar ImpedanceApp y
registrar MuxApp sin asignación dinámica. No se añadieron mediciones, BLE, web
ni CN0349 Manager; no se avanzó a Hito 1D.

**Archivos:** driver en `lib/ADG715/`; aplicación en `src/mux/`; integración en
`src/main.cpp`, `src/terminal/` e `include/config.h`; documentación en
`docs/architecture.md`, `lib/README.md`, `src/README.md`, `test/README.md`,
`lib/ADG715/README.md`, `src/mux/README.md` y `test/hito1c.md`.

**Compilación:** `pio run -e esp32-s3-devkitc-1-n32r8v` finalizó con `SUCCESS`.
RAM: 21.928/327.680 bytes (6,7 %); flash: 360.490/13.107.200 bytes (2,8 %).
**Validación física reportada por el usuario:** `adg715 help`, `ping`, `status`,
lectura inicial `0x00`, `set CAL_LOW_1K` con lectura/verificación `0x12`, y
`set MEASURE_HIGH` con readback `0x81`. Los resultados prueban comunicación y
estado del registro en esos comandos, no conducción analógica. Sigue pendiente
restaurar el estado inicial: `0x00` no está permitido por `write`; el plan
documenta esa limitación y las pruebas restantes en `test/hito1c.md`.

### Confirmación posterior: `clear`

El usuario confirmó que `clear` limpia el monitor después de establecer
`monitor_filters = direct`. La incidencia se considera resuelta; no requiere
modificación del firmware ni volver a cargarlo.

### Resultados de diagnóstico AD5934 reportados por el usuario

El usuario ejecutó `help`, `ad5934 help`, `ad5934 ping`, `ad5934 dump` y `clear`
en hardware; los comandos responden y el dump terminó con 17 bytes. Ping obtuvo
ACK en `0x0D` (sin identificar el IC).

Valores reportados: control `0x80=0x21`, `0x81=0x00`; start frequency
`0x20C49B`; incremento `0x000000`; incrementos `0x0000`; settling `0x000A`;
status `0x72`; Real `0x0F10`; Imaginary `0xDEC6`. Los datos Real/Imaginary son
crudos y no se interpretan como medida.

**Atención para continuar el diagnóstico:** el nibble de modo D15:D12 del valor
de control leído (`0x2`) corresponde a Start Frequency Sweep en la tabla 10 de
Rev. E. Por política de seguridad, ImpedanceApp permite escribir solo si observa
modo `0xA` o `0xB` y Control Low=`0x08`; con `0x2100` rechazará escrituras sin
enviar registros. La salida informada `Control Low=0x00` tampoco coincide con
los bits reservados que Rev. E indica para ese byte. No continuar con pruebas de
escritura hasta aclarar el estado observado. Volver a leer `ad5934 read16 0x80`,
`ad5934 read 0x81` y `ad5934 status`, anotar los resultados y confirmar el
comportamiento tras un ciclo de alimentación controlado. No emitir comandos de
control ni asumir que ACK/dump prueban identidad o medida.

**Estado registrado al revisar la salida:** ping, lecturas, dump, estado, rechazo
seguro de escritura y comandos base observados en hardware. No se infiere una medida
de conductividad a partir del dump.

## 2026-10-26 — Comando dedicado de transición a Standby

Ante la lectura física de control `0x2100`, se añadió `ad5934 standby`. Esta
operación lee el control, conserva sus bits D11:D8 (rango de salida y PGA), cambia
solo el nibble de modo a `0xB`, establece Control Low en `0x08` (D3 reservado a 1,
resto reservado a 0) y verifica ambos bytes por relectura. No ofrece escritura RAW
ni inicia adquisición. Si ya está en power-down, mantiene ese modo; si Standby ya
está correcto, no reescribe. La forma de onda deja de estar en modo de barrido y
VOUT/VIN quedan conectados internamente a tierra según Rev. E.

La compilación PlatformIO tras añadir la operación y su integración terminó con
`SUCCESS` (14,52 s), RAM 21.888/327.680 bytes (6,7 %) y flash 354.462/13.107.200
bytes (2,7 %). La operación aún no se ha ejecutado en hardware. Su verificación
física esperada para el control reportado es `0xB108`. Esta es la situación al
preparar 0.2.0 y no afirma el resultado posterior. Se fijó la identificación de
firmware en `0.2.1` para distinguir la versión con este comando;
esa compilación terminó con `SUCCESS` en 6,70 s, con las mismas cifras de RAM y flash.

El usuario declaró el Hito 1B finalizado después de preparar la versión inicial. Se
conserva esa declaración como cierre del hito; en el chat no se aportó la salida
de la ejecución de `ad5934 standby` ni de la restauración de configuración, así
que no se registran esos valores como evidencia textual.

El usuario probó `ad5934 write24 0x82 0x123456`; la aplicación contestó
«Rechazado: se requiere power-down/standby y Control Low=0x08. No se cambia el
modo». Esto confirma el rechazo preventivo. A continuación, `ad5934 status`
devolvió `0x72`: D1=1 (el IC indica datos Real/Imaginary válidos) y D2=0
(barrido no completado), coherente con el dump. Lecturas directas de control
`ad5934 read16 0x80` y `ad5934 read 0x81` aún pendientes; no se habilitan más
escrituras mientras tanto.

## 2026-10-26 — Corrección de Standby y Power-down

**Solicitud y revisión:** se corrigió `ad5934 standby` para que, si Control High
ya está en power-down (`0xA0`), normalice Control Low a `0x08` sin escribir High.
La tabla 9 del datasheet Rev. E define D3 reservado a 1 y D7:D4/D2:D0 reservados
a 0; power-down conserva VOUT/VIN conectados internamente a tierra. Por ello la
transición `0xA000` → `0xA008` es la corrección documentada. [Datasheet Rev. E](https://www.analog.com/media/en/technical-documentation/data-sheets/ad5934.pdf)

Para modo activo, se mantiene el paso a Standby conservando D11:D8 de Control High,
se escribe `0x08` en Control Low y se releen los registros. No se activa D4 reset,
DDS ni adquisición. El acceso genérico a registros de control continúa bloqueado;
las condiciones de las escrituras de configuración no cambian.

**Archivos modificados:** `lib/AD5934/AD5934.cpp`, `lib/AD5934/README.md`,
`include/config.h`, `test/hito1b.md` y este diario. Versión 0.2.2.

**Compilación:** PlatformIO `SUCCESS` en 6,70 s; RAM 21.888/327.680 bytes (6,7 %),
flash 354.726/13.107.200 bytes (2,7 %). La lectura de verificación en hardware
`0xA000`→`0xA008` queda pendiente; no se atribuye éxito físico a la compilación.
El plan pide guardar la frecuencia original `0x82`, normalizar Standby, leer
`0x80`, escribir/probar `0x123456`, releer y restaurar el valor anotado.

## 2026-10-26 — Hito 1D.1: primera adquisición DFT de punto fijo (0.4.0)

Se implementó `ad5934 measure` como máquina cooperativa de estados en
ImpedanceApp. El perfil centralizado usa MCLK externo 1 MHz (oscilador de la
placa CN-0349), 2 kHz (`0x041893`), Range 1 / 2 Vpp nominal, PGA ×1,
CAL_LOW_1K (`0x12`), incremento 0, cero incrementos y settling 10 ×1. La
frecuencia DDS sigue `floor(f × 2^27 / MCLK)`; el límite configurable es MCLK/16.
El AD5934 Rev. E no expone un selector interno/externo: la placa aporta MCLK y no
es posible confirmar su presencia por I²C.

La secuencia valida AD5934 y ADG715, exige modo inactivo y Control Low `0x08`,
verifica el preset, configura y relee cada campo byte a byte, emite Initialize,
espera diez periodos configurados en firmware, emite Start Sweep y consulta Status
cada 5 ms. Solo D1 permite leer REAL/IMAG. Convierte valores con signo, calcula
magnitud DFT sin unidades y termina con `enterStandby()`. Ante error/timeout
intenta standby; no inicia otra adquisición. Status, result y stop permanecen
disponibles durante la operación. No hay fase, calibración ni conversión a ohmios.

**Archivos:** `include/ad5934_math.h`; `lib/AD5934/AD5934.h/.cpp/README.md`;
`src/impedance/ImpedanceApp.h/.cpp/README.md`; `src/mux/MuxApp.h/.cpp/README.md`;
`src/main.cpp`, `include/config.h`, `platformio.ini`, `docs/architecture.md`,
`include/README.md`, `test/hito1d1.md`, `test/test_ad5934_math/test_main.cpp`.

**Validación de software:** `pio run` compiló con `SUCCESS` en 6,83 s; RAM
22.040/327.680 bytes (6,7 %), flash 369.310/13.107.200 bytes (2,8 %). Se preparó
una suite Unity host para palabra DDS, límites, signed 16-bit, magnitud y máscaras
de control/settling/transiciones. No se pudo
ejecutar porque el equipo no tiene `gcc`/`g++`; queda documentado en
`test/hito1d1.md`. No hay validación física 1D.1 todavía.

**Pruebas físicas preparadas:** ping, config, start/status/result, status AD5934,
read ADG715 y al menos cinco adquisiciones sobre la resistencia interna de 1 kΩ.
Registrar REAL, IMAG, magnitud, duración, D1 y errores. Añadidos casos de límites,
stop, cambios mux durante busy, pérdida de MCLK/I²C y recuperación. No inferir
precisión de impedancia ni conductividad a partir del código DFT.

**Estado:** implementación y compilación terminadas; pruebas unitarias host y
validación física pendientes. No se inició el Hito 1D.2.

### Intento físico 1D.1 reportado por el usuario

`ad5934 measure start` seleccionó y verificó `CAL_LOW_1K` (`0x12`) y configuró
la adquisición, pero terminó con `timeout esperando D1 de datos DFT`. El firmware
indicó que no capturó error I²C. No se obtuvo REAL, IMAG ni magnitud válidos; la
adquisición física no está aceptada. La salida compartida no confirma el estado
final de recuperación a standby.

Para aislar la causa sin cambiar registros, leer en este orden:

```text
ad5934 measure status
ad5934 read16 0x80
ad5934 read 0x81
ad5934 read24 0x82
ad5934 read24 0x85
ad5934 read16 0x88
ad5934 read16 0x8A
ad5934 status
adg715 read
```

Confirmar además con osciloscopio o contador que MCLK en el pin del AD5934 sea
1 MHz. ACK en `0x0D` y en `0x48` no confirma el reloj. El plan de diagnóstico y
las pruebas físicas pendientes están en `test/hito1d1.md`.

## 2026-10-26 — Hito 1D.1: corrección de secuencia de standby (0.4.1)

Las lecturas físicas posteriores al timeout mostraron Control=`0xB108`, frecuencia
`0x041893`, incremento `0`, número de incrementos `0`, settling `0x000A`, Status
`0x70` (D1=0) y ADG715=`0x12`. Se confirmó que la recuperación terminó en standby
y que la configuración de registros coincidía con el perfil. No hubo error I²C,
pero tampoco resultado DFT válido.

La revisión frente a la Figura 24 del datasheet halló una omisión de secuencia en
0.4.0: tras configurar el control Range/PGA en power-down y los parámetros, el
firmware enviaba Initialize sin pasar explícitamente a standby. Se añadió
`wakeToStandby()`, que cambia solo el modo de Power-down a Standby, preserva rango,
PGA y Control Low=`0x08`, y verifica readback. Ahora se configura Range/PGA en
power-down, se verifica Standby, luego se programan los parámetros y se envía
Initialize. No se activa reset ni se altera el requisito de reloj externo.

Versión 0.4.1. Compilación ESP32-S3 con `SUCCESS` en 7,66 s; RAM 22 040 bytes
(6,7 %) y Flash 369 606 bytes (2,8 %). Esta compilación valida la integración,
pero por sí sola no valida la adquisición. En la primera repetición física tras
cargar 0.4.1, `ad5934 measure start` terminó en 176 ms con D1 válido: REAL
`-3405`, IMAG `-7513`, magnitud DFT `8248.587`, sin error, y estado final
standby. `measure status` indicó COMPLETE y `measure result` devolvió los mismos
datos. `adg715 status/read` confirmaron registro `0x12` (CAL_LOW_1K). Es un código
DFT crudo, no una medida en ohmios; la ruta analógica no queda verificada por la
lectura digital del multiplexor. La corrección superó una prueba física inicial;
faltan cuatro repeticiones para completar la serie de cinco.

## 2026-10-26 — Hito 1D.1b: preset configurable de calibración (0.4.2)

**Objetivo:** seleccionar la resistencia interna CN-0349 que se aplicará al
iniciar cada adquisición DFT sin modificar el driver genérico ADG715.

**Cambios:** `ad5934 measure set preset <NAME>` cambia solo la configuración RAM.
El inicio valida el identificador, confirma que AD5934 está inactivo y entonces
aplica/readback el preset mediante MuxApp antes de arrancar el DFT. Se admiten
`CAL_LOW_1K` (`0x12`, RFB/RCAL 1 kΩ/1 kΩ), `CAL_HIGH_1K` (`0x11`, 100 Ω/1 kΩ) y
`CAL_LOW_10K` (`0x22`, 1 kΩ/10 kΩ), conforme a la tabla CN-0349. La tabla MuxApp
es la fuente única de nombres, máscaras y metadata; solo esos tres tienen un ID
de calibración. Los presets manuales ADG715 no cambian la selección lógica. El
resultado conserva el preset usado aunque se modifique la configuración después.
No se añadió persistencia: el arranque usa CAL_LOW_1K. Un fallo al seleccionar
cancela antes de la DFT e intenta recuperar standby. No se implementó barrido.

**Archivos:** `src/impedance/ImpedanceApp.{h,cpp}`; `src/mux/MuxApp.{h,cpp}`;
`include/config.h`; documentación `src/impedance/README.md`, `src/mux/README.md`,
`docs/architecture.md`, esta entrada del diario y `test/hito1d1b.md`.

**Compilación:** ESP32-S3 PlatformIO `SUCCESS` en 8,75 s; RAM 22 088 bytes,
Flash 370 598 bytes. Esto valida integración/compilación, no el hardware ni DFT.

**Pruebas:** el usuario aportó tres adquisiciones físicas previas de
CAL_LOW_1K: (-3405,-7513; 8248.587), (-3404,-7514; 8249.086) y (-3404,-7515;
8249.996). No se ejecutaron con el nuevo firmware. Pendiente cargar 0.4.2 y
realizar selección, adquisición y readback en `0x12`, `0x11` y `0x22`; comprobar
rechazo de nombres no permitidos y que el resultado conserva su preset. No se
afirman resultados DFT esperados para las otras resistencias.

**Limitaciones:** el readback ADG715 confirma bits de switches, no continuidad
analógica; la magnitud sigue siendo código DFT crudo. No calcula gain/offset ni
impedancia/conductividad. RTD, MEASURE, ALL_OFF y máscaras arbitrarias quedan
fuera de la selección de adquisición. Sin NVS, los cambios se pierden al reiniciar.

### Ampliación: los cuatro puntos CN-0349 (0.4.3)

A petición del usuario, se amplió la selección para incluir también
`CAL_HIGH_100R` (`0x09`, RFB=100 Ω y RCAL=100 Ω). CN-0349 lo clasifica como
calibración del rango alto; junto a `CAL_HIGH_1K` (`0x11`) forma sus dos puntos.
`CAL_LOW_1K` (`0x12`) y `CAL_LOW_10K` (`0x22`) son los dos puntos del rango bajo.
La API/tabla única de MuxApp asigna ahora el cuarto ID al preset documentado;
RTD, MEASURE, ALL_OFF y máscaras arbitrarias permanecen excluidos del selector.

Firmware pasa a 0.4.3. Compilación ESP32-S3 PlatformIO `SUCCESS` en 16,11 s;
RAM 22 088 bytes y Flash 370 598 bytes. Es compilación, no validación física.
Queda probar en placa las cuatro rutas y registrar los códigos DFT observados sin
valores previstos. Procedimiento: `test/hito1d1b.md`.

## 2026-10-26 — Hito 1D.2: barrido de frecuencias (0.5.0)

**Objetivo y cambios:** se amplió la máquina DFT existente para configurar e
iniciar barridos sin bloqueo, hasta 64 puntos. Valores por defecto: start 1 kHz,
step 250 Hz, 8 increments/9 puntos, fin nominal 3 kHz. Reutiliza la selección de
preset 1D.1b. Configura y relee start, step, increments y settling; espera D1 por
punto, lee REAL/IMAG antes de Increment Frequency y guarda magnitud y fase
atan2(IMAG, REAL). Al final consulta D2 una vez y recupera standby. Cancelaciones
y errores conservan el prefijo válido y se etiquetan parciales. No envía un
incremento extra ni espera D2 indefinidamente.

**Archivos:** `src/impedance/ImpedanceApp.{h,cpp}`, `lib/AD5934/AD5934.h`,
`include/ad5934_math.h`, `include/config.h`, pruebas host en
`test/test_ad5934_math/test_main.cpp`; documentación en `lib/AD5934/README.md`,
`src/impedance/README.md`, `src/README.md`, `include/README.md`,
`docs/architecture.md` y `test/hito1d2.md`.

**Compilación:** ESP32-S3 PlatformIO `SUCCESS` en 9,01 s; RAM 25 264 bytes,
Flash 376 886 bytes. Esto no valida el hardware.

**Pruebas de software:** añadidos casos host para conteo increments/puntos,
límite 64, incremento cero, frecuencia final, suma DDS de 24 bits, frecuencia
efectiva, signo y fase. `pio test -e native` intentado, pero no pudo compilar:
este equipo no tiene `gcc`/`g++` (`gcc no se reconoce`). No se afirman pruebas
host superadas.

**Validación física:** el barrido predeterminado 0.5.2 completó 9/9 puntos y
reportó standby; ver `test/hito1d2.md`. Pendientes: repetición, cambio de preset,
rechazo de parámetros inválidos, cancelación parcial y regresión de medida única.
La tabla contiene DFT cruda, no una medición de impedancia calibrada.

**Limitaciones:** tabla limitada a 64 puntos en RAM; magnitud y fase son DFT
cruda, sin impedancia/conductividad ni corrección de fase; no hay NVS/BLE ni
validación física. El Manager CN0349 y Hito 1E siguen futuros.

### Corrección tras la primera ejecución 1D.2 (0.5.1)

El usuario configuró el barrido por defecto correctamente (1–3 kHz, step 250 Hz,
8 increments/9 puntos; DDS start `0x020C49`, step `0x008312`). Se registró la
selección/readback de CAL_LOW_1K (`0x12`), pero al emitir el primer Increment
Frequency el driver rechazó la transición de control en validación (`0x80`),
por lo que no hubo puntos de barrido reportados.

La validación previa asumía que el control conservaría operación `0x2`. Se
ajustó para aceptar `No operation` (`0x0`) o `Start Frequency Sweep` (`0x2`), y
el driver ahora exige además D1 activo antes de cada Increment. Esto preserva la
condición de que el punto actual esté listo antes de avanzar. Versión 0.5.1;
compilación ESP32-S3 `SUCCESS` en 9,88 s (RAM 25 264 bytes, Flash 376 926 bytes).
Esto es compilación; las pruebas físicas siguen pendientes. El próximo ensayo
debe confirmar progreso más allá del primer punto, D2 al final y standby.

### Segundo intento y corrección de transición (0.5.2)

El usuario probó 0.5.1 con la misma configuración predeterminada y el primer
Increment Frequency volvió a rechazarse en validación en `0x80` (bytes
completados: 1). La condición que aceptaba ciertos bits de modo retenidos no
representaba de forma fiable el estado de secuencia tras leer REAL/IMAG.
Conforme al flujo de barrido de la Figura 24 del datasheet Rev. E, el driver
ahora requiere D1 válido y Control Low `0x08`, pero no infiere del nibble de
operación retenido si el punto fue leído; ImpedanceApp solo envía el incremento
después de leer y guardar ambos datos. Se conservan los demás controles de
escritura y los bits de rango/PGA.

Firmware 0.5.2 compila para ESP32-S3 (`SUCCESS`, 7,89 s; RAM 25 264 bytes,
Flash 376 918 bytes). La primera repetición física del barrido predeterminado
completó 9/9 puntos, desde 999,995 hasta 2 999,969 Hz, y reportó el AD5934 en
standby. El resultado se conserva en `test/hito1d2.md`.

## Hito 1E — robustez, diagnóstico integrado y recuperación

**Fecha:** 2026-10-26. **Versión:** 0.6.0. **Objetivo:** exponer estado fiable
del subsistema, clasificar errores, bloquear operaciones incompatibles y ofrecer
diagnóstico/recovery manual sin reinicio ni cambios automáticos de preset.

**Arquitectura:** `ImpedanceApp` conserva la coordinación de AD5934/ADG715 y la
exclusión con su máquina DFT. Los drivers mantienen las transacciones tipadas y
readback; `MuxApp` expone a la capa superior ping, lectura de máscara y consulta
de preset conocido. Terminal solo despacha y presenta. `hardwareReady` exige
comunicación de ambos dispositivos, registros relevantes válidos, estado seguro,
diagnóstico/recovery verificado y máscara ADG715 conocida; ACK por sí solo no lo
establece.

**Cambios:** añadidos `ImpedanceStatus`, códigos tipados mínimos para busy,
argumentos/estado, NACK/timeout/error I2C, verificaciones, timeouts DFT/barrido,
datos inválidos y recuperación fallida. Se conserva último error, activo,
contador por código, subsistema, fase de aplicación, millis, detalle,
recuperabilidad y resultado de recovery. Nuevos comandos: `ad5934 diag`,
`ad5934 status`, `ad5934 errors`, `ad5934 errors clear` y `ad5934 recover`.
El diagnóstico lee sin conmutar ni medir; recovery sondea, verifica y normaliza
AD5934 a standby, mantiene el estado actual del ADG715 y solo declara éxito tras
readback. Medición requiere readiness verificado. `measure sweep stop` conserva
el prefijo válido y el standby se confirma antes de liberar busy. No hay reinicio,
reintentos ilimitados, medición automática ni comandos de fallo en producción.

**Archivos principales:** `include/impedance_diagnostics.h`, `include/config.h`,
`src/impedance/ImpedanceApp.{h,cpp}`, `src/mux/MuxApp.h` y prueba Unity de nombres
del modelo. Documentación: arquitectura y READMEs de drivers, mux y app.

**Compilación final:** PlatformIO ESP32-S3 `SUCCESS`, 6,95 s; RAM 25 360 bytes,
Flash 383 418 bytes. El watchdog de fases de control/configuración es 1500 ms;
la espera de DFT es 500 ms y Wire mantiene 25 ms por transacción.
**Automatizadas:** añadida prueba unitaria de nombres/default
del modelo tipado. `pio test -e native` intentado, sin compilar porque el entorno
no dispone de `gcc`/`g++`; por tanto ningún test host se declara superado.

**Físicas pendientes:** ejecutar `ad5934 diag` dos veces y comparar que no cambia
la máscara; stop inactivo; cancelación activa y prefijo; errores/configuración
inválida y operaciones concurrentes; pérdida/timeout de DFT y fallos de lectura,
escritura/readback; recuperación; medida única y barrido tras recovery. No se
introdujeron inyecciones de fallo de producción. La integración de mocks de bus
para ejercitar estos fallos queda pendiente.

**Validación física inicial reportada por el usuario (0.6.0):** `ad5934 diag`
devolvió READY, ACK en ambos dispositivos, control seguro, configuración válida,
Status `0x72` y máscara conocida `0x22`. `ad5934 status` coincidió y `errors`
mostró todos los contadores en cero; `errors clear` respondió sin modificar
hardware. El barrido predeterminado completó 9/9 y notificó standby. Durante el
barrido ImpedanceApp aplicó CAL_LOW_1K (`0x12`); luego `ad5934 recover` verificó
standby y conservó esa máscara. La prueba no es diagnóstico repetido, fallo
inyectado ni verificación eléctrica analógica.

**Limitaciones:** transacciones Wire siguen siendo síncronas con timeout de 25 ms;
no hay concurrencia entre maestros. El diagnóstico/recovery del comando puede
realizar varias lecturas acotadas en la misma llamada. DFT sigue sin conversión a
impedancia/calibración y el readback ADG715 no demuestra la ruta analógica.
Planificación con Hito 2 sigue sin iniciarse.

**Estado:** diagnóstico, estado, limpieza, barrido y recovery tienen una primera
validación física exitosa. Restan los escenarios de fallo, cancelación y la suite
host con mocks. Ver [`test/hito1e.md`](test/hito1e.md).

## Hito 2A — CN0349Manager: arquitectura, API e integración

**Fecha:** 2026-10-26. **Versión:** 0.7.0. Se creó `src/cn0349/CN0349Manager.{h,cpp}` como fachada tipada de alto nivel. `ImpedanceApp` sigue siendo el único motor y dueño del control hardware. Manager recibe referencia a la instancia existente; no duplica drivers, no inicializa Wire y su `update()` avanza el motor DFT compartido.

La API expone presets solicitados, configuración single/sweep con validación delegada a las funciones AD5934 existentes, start/cancel, status, resultados DFT RAW, snapshots de diagnóstico, diagnóstico activo y recuperación. Los resultados de single y sweep separan frecuencias solicitadas y DDS efectivas. El estado `IDLE` queda separado del último outcome. Las consultas de estado/config/diagnóstico son cacheadas y no ejecutan I2C oculto; `diag` y `recover` sí son acciones explícitas.

Se añadieron `cn0349 help/status/config/preset list/preset set/measure single/measure sweep/measure stop/result/diag/recover`. Se conservaron `ad5934` y `adg715`. El Manager toma configuración del ImpedanceApp y la máscara observada de MuxApp para mantener sincronía tras comandos heredados. Los cuatro presets siguen definidos en una sola tabla MuxApp.

**Archivos:** nuevos `src/cn0349/CN0349Manager.h/.cpp`, `src/cn0349/README.md`, `test/hito2a.md`; modificados `include/config.h`, `src/main.cpp`, `src/terminal/Terminal.{h,cpp}`, `src/impedance/ImpedanceApp.{h,cpp}`, `src/mux/MuxApp.{h,cpp}`, documentación de arquitectura, terminal, mux, impedancia e índice de tests.

**Compilación:** PlatformIO ESP32-S3 `SUCCESS`, 7,58 s; RAM 27 208 bytes y
Flash 391 138 bytes. **Host:** `pio test -e native` intentado, no compiló por
falta de `gcc`/`g++`; tampoco existe mock de ImpedanceApp/TwoWire, así que no se
declara cobertura automatizada del Manager. **Físicas:** pendientes; seguir
`test/hito2a.md` para manager, single, sweep, cancel, recovery y regresión de
comandos antiguos.

No se añadieron calibración, ohmios, conductancia, conductividad, temperatura, persistencia, BLE ni webapp. El preflight de start ejecuta unas transacciones I2C síncronas acotadas; la espera/conversión y el barrido se mantienen asíncronos en el update del motor.

**Estado:** integración compilable; validación física y tests host de integración pendientes. No avanzar al Hito 2B.

## Hito 2B — Calibración electrónica de impedancia

**Fecha:** 2026-10-26. **Versión:** 0.8.0. El `CN0349Manager` coordina series de cinco adquisiciones DFT para cada preset RCAL, calculadas con el motor único de `ImpedanceApp`. Cada muestra conserva REAL, IMAG, magnitud y fase. Al completar la serie se almacenan en RAM media, desviación estándar muestral, variación relativa, media circular de fase y GF. Se conserva una entrada independiente para cada ruta y una calibración anterior permanece intacta si una serie nueva falla o se cancela.

**Modelo:** `GF = 1 / (RCAL × Mmedia)`. Bajo el modelo de transferencia inversamente proporcional a impedancia y suponiendo una RCAL nominal conocida y cadena lineal estable, el GF relaciona el código DFT con el recíproco de impedancia. La fase guardada es una referencia instrumental, no una fase física de celda. Se almacenan MCLK, frecuencia solicitada/efectiva, RFB, RCAL, rango de excitación, PGA y settling. Un getter marca si coincide con la configuración actual; cada preset mantiene su propio coeficiente. No se hace conversión de resultados de celda en 2B ni se ajustan correcciones empíricas.

**Terminal:** `cn0349 cal help/status/run <PRESET|all>/result/list/stop/clear`. `run all` procesa los cuatro presets de forma secuencial sin esperar conversiones en el handler; cada adquisición avanza por el `update()` existente. `clear` solo borra RAM. `cal result` informa GF, unidades, dispersión y configuración; GF no está expresado en ohmios. `cal list` separa disponibilidad y compatibilidad.

**Archivos:** ampliado `src/cn0349/CN0349Manager.{h,cpp}` con API tipada, almacenamiento fijo en RAM y estadísticas; actualizado `include/config.h` a 0.8.0 / 2B; documentación de arquitectura, módulo y plan manual en `test/hito2b.md`.

**Compilación:** PlatformIO ESP32-S3 `SUCCESS` en 7,67 s; RAM 27 912 bytes y Flash 401 134 bytes. `pio test -e native` se intentó, pero no compiló porque no están disponibles `gcc`/`g++`. No se ha ejecutado una calibración física en esta entrega; las cuatro rutas, repetibilidad, cancelación, conservación tras fallo y compatibilidad con adquisiciones previas siguen pendientes de prueba en placa.

**Limitaciones:** RCAL/RFB son nominales; tolerancias, parásitos, impedancia de salida, fase analógica y deriva pueden afectar GF. El promedio y su dispersión describen las muestras, pero no imponen criterio de aceptación ni eliminan atípicos. Una calibración electrónica sobre resistencias internas no caracteriza una celda, cableado ni interfaz electroquímica. La fase se conserva sin interpretarla como fase física. No hay persistencia NVS.

**Estado:** integración compilada; validación física pendiente. No avanzar al Hito 2C hasta revisar resultados y confirmar el plan experimental.

### Validación física añadida — 2026-10-26

El usuario confirmó una calibración individual `CAL_HIGH_100R` de cinco muestras (`COMPLETE`, sin error): magnitud media 8194,710, desviación muestral 0,917, variación relativa 0,011 %, fase instrumental media −114,315° y GF 0,000001220. Frecuencia efectiva reportada 1999,996 Hz; standby al terminar.

También se completó `cn0349 cal run all`: el estado final fue `COMPLETE`, `CAL_LOW_10K`, muestra 5/5, secuencia all y error NONE. Las veinte adquisiciones reportaron D1 válido y standby. Magnitudes mostradas en la terminal por serie (código DFT, tres decimales):

- `CAL_HIGH_100R`: 8195,987; 8194,897; 8194,310; 8193,811; 8192,813.
- `CAL_HIGH_1K`: 859,142; 859,559; 859,559; 859,559; 859,559.
- `CAL_LOW_1K`: 8249,826; 8250,653; 8249,742; 8250,155; 8249,245.
- `CAL_LOW_10K`: 838,955 en las cinco muestras mostradas.

`cal list` confirmó los cuatro slots válidos y compatibles, con GF mostrados redondeados: 0,000001220; 0,000001164; 0,000000121; 0,000000119. `cal result` para LOW_10K reportó media 838,955, desviación y variación 0,000 a la resolución de DFT almacenada/impresa, fase −114,732°, GF 0,000000119, ID 13 y duración 1142 ms. Una dispersión reportada como cero no demuestra ausencia de ruido por debajo de la cuantización del convertidor.

Los resultados son coherentes con las referencias orientativas y muestran repetibilidad de código DFT para esta ejecución. No validan tolerancia eléctrica, ruta analógica, impedancia externa ni modelo de celda. Stop/cancel, fallo inyectado/conservación tras fallo, `clear`, reinicio y regresión física completa siguen sin evidencia reportada.

**Estado actualizado:** cuatro calibraciones electrónicas físicas y una secuencia all confirmadas; compilación ESP32-S3 aprobada. Aún no se afirma exactitud de impedancia de celda ni se avanza al Hito 2C.

## Hito 2C.1 — Impedancia externa aparente por ruta Pt100

**Fecha:** 2026-10-27. **Versión:** 0.9.0. **Objetivo:** primera conversión a Ω de
una adquisición externa a 2000 Hz usando MEASURE_PT100/RTD `0x41`, RFB 100 Ω.

**Arquitectura y cambios:** CN0349Manager integra Impedance Measurement junto
al Calibration Engine 2B, sobre el mismo ImpedanceApp y los mismos drivers.
Selección manual entre CAL_HIGH_100R y CAL_HIGH_1K, validación de frecuencia/DDS,
MCLK, excitación, PGA, settling y RFB; rechazo de LOW y slots ausentes.
Adquisición cooperativa existente, selección/readback RTD y final en standby
verificado antes de COMPLETE. Se conservan DFT, GF, ID/máscara/fase de calibración,
frecuencias solicitada/efectiva, configuración, duración y errores.
El intento actual y el último válido se conservan por separado; rechazos previos
no sobrescriben la medida. Las calibraciones 2B permanecen intactas.

**Modelo:** `Z_aparente = 1/(GF*Mmedida)`, con GF de 2B
`1/(RCAL*Mcal_media)`. Fase corregida instrumentalmente por resta de fase de
calibración, normalizada a [-180°, +180°]. No se afirma resistencia externa exacta
ni fase física validada. No se restan 100 Ω ni se modifica GF con valores esperados.
[Circuito, fuentes, unidades e hipótesis](docs/pt100-route.md): se revisó la nota
oficial, pero el esquema de fabricación no estaba en el repositorio y el paquete
original redirigió a descarga protegida; su revisión completa y la comprobación
del pinout físico quedan pendientes. Se continúa con medida aparente sin corrección.

**Archivos de esta entrega:** nuevos `src/cn0349/CN0349Impedance.cpp`,
`src/cn0349/ImpedanceMath.h`, `docs/pt100-route.md`, `test/hito2c1.md` y
`test/host_cn0349/{Arduino.h,Wire.h,test_main.cpp,run.cmd,README.md}`;
ampliados `src/cn0349/CN0349Manager.{h,cpp}`, `src/impedance/ImpedanceApp.{h,cpp}`,
`src/mux/MuxApp.{h,cpp}`, `include/config.h`, documentación de esas carpetas,
`src/README.md`, `docs/architecture.md`, `test/README.md` y este diario.
Se conservaron los cambios anteriores sin commit. No se modificaron drivers,
main ni parser Terminal para esta entrega.

**API pública:** selección/calibración consultable, start/cancel, running,
resultado actual, último resultado válido y error de solicitud. Firmas y semántica
en [README del Manager](src/cn0349/README.md).

**Comandos:** `cn0349 impedance help|config|cal list|cal set <CAL_HIGH_100R|CAL_HIGH_1K>|start|status|result|stop`.
`result` identifica ruta/calibración/configuración y presenta datos originales,
fases, GF, Ω aparentes y validez; distingue el válido anterior de un intento fallido.

**Pruebas y resultados:** compilación ESP32-S3 PlatformIO SUCCESS en 7,50 s; RAM 28 184 bytes, Flash 408 162 bytes. Suite host MSVC
con código real y Stream/reloj/I²C simulados: matemática PASS,
manager/terminal/regresión PASS y fallos readback/standby PASS. Incluye los cuatro
slots 2B, ambas HIGH, errores, busy, cancelación/reanudación, DFT cero/timeout,
consultas sin I²C y regresión single/sweep/diag. Detectó y corrigió una marca
`cancelled_` heredada entre adquisiciones. La comparación DDS contempla solo
redondeo float de las cinco frecuencias acumuladas por 2B. `pio test -e native`
no compiló por ausencia de gcc/g++; no se declara aprobado. La suite host usa
MSVC instalado y no sustituye la verificación física. No se ha subido firmware.

**Ensayo físico:** medir cada resistencia nominal 220 Ω y 2200 Ω con multímetro,
confirmar J1/PT100 en placa y esquema, conectar individualmente y medir cada
una con las dos calibraciones guardando DFT/fases/GF/configuración/errores.
[Procedimiento completo y tabla pendiente](test/hito2c1.md).

**Limitaciones:** modelo inicial, RCAL/RFB nominales, ruta analógica y parásitos
sin caracterizar; no hay detección fiable de saturación/señal insuficiente ni
precisión absoluta demostrada. La validez es instrumental/numérica. Preflight y
transacciones I²C siguen siendo síncronos acotados; espera DFT no bloqueante.
Un fallo de comunicación puede impedir standby y produce FAILED. RAM sin
persistencia, sin autorango, temperatura, correcciones empíricas, BLE ni webapp.

**Estado:** implementación compilable y comprobada con simulación; preparada
para validación física una vez comprobados esquema/pinout. Medidas reales de
220 Ω, 2200 Ω y comparación entre calibraciones pendientes. No se avanza a 2C.2.


### Primera medida externa reportada — 2026-10-27

El usuario conectó una resistencia nominal de 220 Ω, medida con multímetro en
**215 Ω**, y aportó el [transcript de firmware 0.9.0](test/hito2c1-220r-transcript.txt).
`recover` y `diag` reportaron COMPLETE/READY; las veinte adquisiciones de
`cal run all` mostraron D1 válido y standby. `impedance cal list` confirmó
HIGH_100R ID 1/GF 0,000001220961 y HIGH_1K ID 2/GF 0,000001164391 compatibles.
La adquisición externa usó **CAL_HIGH_100R**, máscara de medida RTD `0x41`.

| Dato | Resultado reportado |
| --- | --- |
| Referencia multímetro | 215 Ω (sin incertidumbre/resolución especificadas) |
| Frecuencia solicitada / DDS efectiva | 2000 / 1999,996582 Hz |
| REAL / IMAG | −1607 / −3542 |
| Magnitud DFT | 3889,500244 |
| Fase DFT / calibración | −114,403725° / −114,405861° |
| Fase corregida instrumentalmente | +0,002136° |
| Impedancia aparente | **210,573898 Ω** |
| Diferencia frente a 215 Ω | **−4,426102 Ω (−2,05865 %)** |
| Resultado / error | COMPLETE / VALID / NONE |
| Finalización | Standby reportado; duración Manager 226 ms |

Esta ejecución confirma una primera adquisición externa y conversión numérica
con HIGH_100R. La discrepancia observada es aproximadamente −2,06 % respecto
al valor indicado por el multímetro; no constituye una especificación de precisión.
Una sola muestra no permite determinar repetibilidad ni atribuir la diferencia
a RCAL, cableado o ruta analógica. La fase cercana a cero es compatible con el
ensayo resistivo, pero continúa siendo una corrección instrumental sin validación
independiente. No se modifica GF ni se aplica corrección empírica.

Pendientes: repetir esta carga con **CAL_HIGH_1K**, obtener varias adquisiciones
por calibración, ensayar 2200 Ω con su valor real de multímetro, diagnóstico
posterior y demás escenarios físicos del plan. La comprobación independiente del
esquema/pinout físico no queda demostrada por el transcript. No avanzar a 2C.2.


### Segunda calibración sobre la misma carga — 2026-10-27

El usuario aportó una segunda medida de la misma resistencia nominal 220 Ω
(multímetro 215 Ω), seleccionando **CAL_HIGH_1K / 0x11 / ID 2**.
La ruta externa continuó siendo RTD `0x41`, RFB 100 Ω, 2000 Hz solicitados,
1999,996582 Hz DDS efectivos, MCLK 1 MHz, Range 1, PGA x1 y settling 10.

| Dato | HIGH_100R anterior | HIGH_1K nueva |
| --- | --- | --- |
| REAL / IMAG | −1607 / −3542 | −1606 / −3546 |
| Magnitud DFT | 3889,500244 | 3892,730713 |
| GF aplicado | 0,000001220961 | 0,000001164391 |
| Fase DFT / calibración (°) | −114,403725 / −114,405861 | −114,365997 / −114,738792 |
| Fase corregida instrumentalmente (°) | +0,002136 | +0,372795 |
| Z aparente (Ω) | 210,573898 | 220,620987 |
| Diferencia frente a 215 Ω | −4,426102 (−2,05865 %) | +5,620987 (+2,61441 %) |
| Estado / error | COMPLETE / VALID / NONE | COMPLETE / VALID / NONE |
| Duración Manager | 226 ms | 219 ms |

La segunda adquisición reportó D1 válido y standby confirmado. La magnitud DFT
cambió aproximadamente +0,0831 %, mientras la impedancia aparente difiere
10,047089 Ω entre ambas selecciones. Bajo la fórmula implementada, la diferencia
está dominada por los distintos GF; estas dos adquisiciones no determinan su
causa física ni una estadística de repetibilidad. Ambas estimaciones quedan a
lados opuestos de la referencia indicada por el multímetro. No se promedian los
GF/resultados para corregir la carga ni se elige una calibración por cercanía al
valor nominal. La fase continúa siendo instrumental.

**Estado actualizado:** primera comparación física entre las dos calibraciones
para la carga nominal 220 Ω documentada. Pendientes varias repeticiones por
calibración, carga nominal 2200 Ω medida con multímetro y resto del plan físico.
Sin cambios de firmware ni avance a 2C.2.


### Carga nominal 2,2 kΩ, primera medida — 2026-10-27

El usuario reportó una resistencia nominal 2,2 kΩ medida con multímetro en
**2,15 kΩ (2150 Ω)**, sin resolución/incertidumbre del instrumento especificadas.
La medida externa utilizó CAL_HIGH_1K, máscara de calibración 0x11, ID 2,
GF 0,000001164391, ruta RTD 0x41/RFB 100 Ω, frecuencia solicitada 2000 Hz,
DDS efectiva 1999,996582 Hz, MCLK 1 MHz, Range 1, PGA x1 y settling 10.

| Dato | Resultado reportado |
| --- | --- |
| REAL / IMAG | −171 / −367 |
| Magnitud DFT | 404,882690 |
| Fase DFT / calibración | −114,982697° / −114,738792° |
| Fase corregida instrumentalmente | −0,243904° |
| Impedancia aparente | **2121,152832 Ω** |
| Diferencia frente a 2150 Ω | **−28,847168 Ω (−1,34173 %)** |
| Estado / error | COMPLETE / VALID / NONE |
| Finalización | Standby reportado; duración Manager 214 ms |

Con el mismo GF HIGH_1K/ID 2, la carga de referencia 215 Ω había dado
220,620987 Ω (+2,61441 %), mientras esta referencia 2150 Ω da −1,34173 %.
El cambio de signo de la desviación entre estas dos muestras no respalda una
corrección multiplicativa única deducida de ellas. Tampoco permite identificar
por sí solo una corrección aditiva, no linealidad o causa analógica. No se modifica
GF ni se aplica corrección. Los valores de multímetro se utilizan únicamente
para comparación externa, nunca como constantes del firmware.

**Estado actualizado:** las dos cargas externas tienen una primera muestra física
con HIGH_1K; la carga 220 Ω tiene además una muestra HIGH_100R. Pendiente medir
la carga 2,2 kΩ con HIGH_100R, repetir adquisiciones, completar diagnóstico posterior
y restantes pruebas físicas. No se afirma precisión absoluta ni avance a 2C.2.


### Comparación de ambas calibraciones en las dos cargas — 2026-10-27

El usuario aportó la medida restante de la carga nominal 2,2 kΩ (multímetro
2150 Ω), ahora con CAL_HIGH_100R / máscara 0x09 / ID 1 / GF 0,000001220961.
La ruta externa sigue siendo RTD 0x41/RFB 100 Ω; frecuencia solicitada 2000 Hz,
DDS efectiva 1999,996582 Hz, MCLK 1 MHz, Range 1, PGA x1, settling 10.

REAL −171, IMAG −367 y magnitud 404,882690 coinciden exactamente a la resolución
reportada con la muestra HIGH_1K anterior. Fase DFT −114,982697°,
fase de calibración −114,405861°, fase corregida instrumentalmente −0,576836°.
Resultado: **2022,875488 Ω**, diferencia **−127,124512 Ω (−5,91277 %)** frente a
2150 Ω. COMPLETE/VALID/NONE, standby reportado, duración Manager 214 ms.

| Referencia multímetro | HIGH_100R: Z aparente / diferencia | HIGH_1K: Z aparente / diferencia |
| --- | --- | --- |
| 215 Ω | 210,573898 Ω / −2,05865 % | 220,620987 Ω / +2,61441 % |
| 2150 Ω | 2022,875488 Ω / −5,91277 % | 2121,152832 Ω / −1,34173 % |

Para la carga 2150 Ω, la diferencia entre resultados es 98,277344 Ω y se debe
al GF aplicado, dado que los datos DFT reportados son idénticos. Esto confirma
la aplicación de coeficientes independientes sin cambiar la ruta externa, pero
no demuestra su exactitud ni la causa física de la discrepancia entre factores.
HIGH_100R queda más cerca de la referencia de 215 Ω y HIGH_1K de la de 2150 Ω
en estas muestras; no se deduce una regla de selección automática ni se promedian
los GF. La precisión de las referencias de multímetro no está especificada.

**Estado actualizado:** primera matriz física 2 cargas × 2 calibraciones completa,
con una adquisición por combinación. El hito sigue en validación experimental:
faltan repeticiones, diagnóstico posterior, comprobación independiente del
esquema/pinout y los restantes escenarios físicos del plan. No hay precisión
absoluta validada, corrección empírica ni avance a 2C.2.

### Hoja de ensayo de repetibilidad — 2026-10-27

Se creó [test/hito2c.1_testing.md](test/hito2c.1_testing.md) con preparación de
sesiones, comandos y espera de finalización, cinco adquisiciones por cada una
de las cuatro combinaciones, tablas y espacios para pegar/comentar las salidas,
diagnóstico posterior y estadística. La nueva serie de veinte medidas está
pendiente; las cuatro medidas preliminares permanecen como evidencia separada.
Se registrarán nuevas calibraciones si se apaga la placa al cambiar la carga.

### Hoja de ensayo simplificada — 2026-10-27

Se simplificó [hito2c.1_testing.md](test/hito2c.1_testing.md) para seguir primero
la carga 2k2 ya conectada y calibrada: cinco medidas por cada HIGH, después cambio
a 220 Ω y nueva calibración. Se mantienen GF recibidos, tablas para las veinte
adquisiciones, salidas originales y comentarios. El detalle anterior queda
[archivado](test/hito2c.1_testing_detallado.md). No hay medidas nuevas en esta actualización.


### Cierre final del Hito 2C.1 — 2026-10-27

A petición del usuario, Hito 2C.1 se da por cerrado tras compilar firmware,
implementar la medida aparente y completar con el usuario una matriz física de
20 adquisiciones: 5 por cada una de dos cargas nominales y dos calibraciones
compatibles. Las veinte salidas reportaron COMPLETE/VALID/NONE y standby; los
diagnósticos finales de las dos sesiones fueron READY, con error/counters a cero
y coeficientes conservados dentro de cada arranque. Estadística y transcripciones
están en [el documento de ensayo](test/hito2c.1_testing.md).

Las cargas tenían banda dorada; los valores de 215 Ω y 2150 Ω son indicaciones de
multímetro reportadas, cuya exactitud, rango, resolución e incertidumbre no se
documentaron. Las diferencias relativas del ensayo se comparan con esas lecturas,
no con patrones trazables. La repetibilidad observada fue alta, pero no se afirma
exactitud absoluta ni causa de las desviaciones. No se ajustaron GF ni se aplicaron
correcciones empíricas. Esquema de fabricación y pinout físico no se verificaron
independientemente. El cierre no da inicio a 2C.2.


### Observación adicional Pt100 — 2026-10-27

Después del cierre de la matriz discreta, el usuario conectó un Pt100 de tres
hilos usando dos conductores en la ruta RTD 0x41. Con HIGH_100R ID 1/GF
0.000001220810, el firmware reportó 108,866821 Ω aparentes, DFT −3093/−6859,
fase corregida instrumental +0,145485°, COMPLETE/VALID/NONE y standby.
Se conserva aparte en [el documento de prueba](test/hito2c.1_testing.md).
No se conoce la asignación de hilos ni se ha separado la resistencia de cable;
no se infiere temperatura ni resistencia propia exacta del elemento Pt100.


Segunda lectura aparente del Pt100 con HIGH_1K: 114,052193 Ω (REAL −3096,
IMAG −6864, magnitud 7529,920898, COMPLETE/VALID/NONE). Comparada con
108,866821 Ω obtenidos con HIGH_100R, Z aparente cambia +5,185372 Ω (+4,76 %)
y la magnitud DFT +0,077 %. Es comparación entre coeficientes, no repetición
independiente ni validación de temperatura; [datos](test/hito2c.1_testing.md).

### Hito 2D — Temperatura Pt100 experimental — 2026-10-27

Objetivo: convertir la resistencia aparente adquirida por CN0349 a Celsius
mediante IEC 60751, manteniendo trazabilidad y el flujo asíncrono existente.

Cambios: conversor independiente [Pt100](src/temperature/README.md), raíz
cuadrática estable para T≥0 y bisección acotada para T<0, rango -200 a +850 °C
y errores explícitos. [Manager](src/cn0349/README.md) incorpora
`cn0349 temperature help|start|status|result|stop` y un snapshot de adquisición,
calibración realmente aplicada/ID, GF, datos DFT, frecuencia, fase, tiempo,
Z aparente y conversión. No genera temperatura válida tras fallo upstream.
La selección posterior y la recalibración no reinterpretan resultados previos.

Uso con calibración RAM compatible ya disponible:

```text
cn0349 impedance cal set CAL_HIGH_100R
cn0349 temperature start
cn0349 temperature status
cn0349 temperature result
```

Esperar a que termine RUNNING antes de recoger el resultado. También admite
CAL_HIGH_1K seleccionado manualmente. Si falta calibración, se informa error;
start no recalibra ni cambia la selección. [Protocolo completo](test/hito2d.md).

Pruebas: firmware ESP32-S3 compilado SUCCESS (RAM 28280 bytes, Flash 410642
bytes). `test\host_cn0349\run_math.cmd`: 3 pruebas Pt100 y 11 AD5934 PASS con
MSVC/Unity. `test\host_cn0349\run.cmd`: cuatro grupos PASS, incluyendo regresión,
fallos, terminal y asociación inmutable. Se corrigió una referencia de prueba
AD5934 truncada más allá de su tolerancia, sin cambiar el driver. `pio test -e
native` intentado: ERRORED por ausencia de gcc/g++; se ejecutaron las mismas
suites matemáticas mediante MSVC. Warning C4018 del driver AD5934 preexistente.

Estado: implementación y validación software completadas; validación física de
temperatura pendiente. No se ha cargado firmware ni ensayado hardware en este
hito. Etiqueta obligatoria `EXPERIMENTAL / NOT ABSOLUTELY CALIBRATED`.
Conectar un rojo a PT100-1, azul a PT100-2 y aislar el otro rojo: tres hilos
usados como dos terminales, con resistencia de cables sumada al elemento.
No hay compensación, offsets, resta de 0,6 ohm, autorange ni cambios de GF.
El rango matemático y la validez DFT no prueban exactitud absoluta.
[Arquitectura global y futura web BLE](docs/architecture.md).

### Arranque automático recover + cal run all — 2026-10-27

Objetivo: dejar la placa preparada para medir después de cada arranque, sin
introducir los comandos de preparación manualmente. Tras inicializar I2C y
comprobar comunicación, `setup` llama a `prepareMeasurements`: ejecuta la
recuperación existente y, solo si termina correctamente, inicia las cuatro
calibraciones RAM a 2000 Hz con cinco muestras por preset. La adquisición avanza
desde `loop`, manteniendo operativo el terminal. La calibración RTD seleccionada
se conserva (por defecto CAL_HIGH_100R).

El mensaje `Sistema preparado para medir` se publica únicamente al completar
la secuencia y verificar compatibilidad de la calibración seleccionada. Si
recover falla, no comienza la calibración; si esta falla o se cancela, se informa
que el sistema no está preparado. No hay reintentos automáticos. Se mantienen
`cn0349 recover`, `cn0349 cal run all`, `cn0349 cal status` y `cn0349 cal stop`.
La preparación automática ocurre al arrancar; `temperature start` sigue sin
recalibrar. Cada reinicio genera nuevas calibraciones, no persistentes ni una
validación de exactitud absoluta.

Pruebas: firmware ESP32-S3 SUCCESS (RAM 28288 bytes, Flash 411498 bytes).
`test\host_cn0349\run.cmd`: cinco grupos PASS, incluido arranque completo,
cuatro slots con cinco muestras, bloqueo de medidas durante calibración,
temperatura inmediatamente después, cancelación, fallo DFT, fallo de recovery
y regresión del comando manual `cal run all`. Pruebas físicas de este arranque
pendientes; no se ha cargado el firmware en placa.

### Temperatura continua cada segundo — 2026-10-27

Objetivo: obtener medidas sucesivas sin repetir comandos. Se añade
`cn0349 temperature continuous`: primera medida inmediata y siguientes con
intervalo nominal de 1000 ms entre inicios, mediante el motor asíncrono existente.
Cada resultado muestra temperatura experimental, resistencia aparente,
calibración/ID, DFT, frecuencia efectiva, fase, duración y errores.

```text
cn0349 temperature continuous
cn0349 temperature status
cn0349 temperature stop
```

Se conserva `temperature start` para medida única y `temperature result` para
auditar el último intento. El modo continuo reserva el subsistema incluso entre
medidas; detenerlo antes de cambiar calibración o iniciar otra operación.
Un fallo o conversión inválida detiene el modo con un aviso. No solapa medidas,
no acumula retrasos ni recalibra. El intervalo depende del avance del loop;
si una adquisición tarda más de un segundo, el siguiente inicio se retrasa.

Pruebas: `test\host_cn0349\run.cmd`, seis grupos PASS, incluyendo cadencia
999/1000 ms, desbordamiento de millis, reserva del bus, parada, timeout y
regresiones anteriores. Firmware ESP32-S3 compilado SUCCESS. Validación física
de la cadencia pendiente; no se ha cargado el firmware. Se mantienen todos los
límites de temperatura experimental y ausencia de compensación de cables.

### Salida continua: solo temperatura — 2026-10-27

A petición del usuario, cada muestra continua muestra únicamente
`temperature: xx.xxºC`, con dos decimales. Se silencian los mensajes informativos
de mux, inicio, DFT, standby y resumen de impedancia durante esas adquisiciones.
El detalle sigue disponible con `cn0349 temperature result`; las medidas únicas
mantienen su salida anterior. La activación conserva el aviso experimental y
los errores siguen visibles. No cambian adquisición, calibración ni cadencia.

Firmware ESP32-S3 compilado SUCCESS (RAM 28296 bytes, Flash 412430 bytes).
Suite host: seis grupos PASS, incluida ausencia de salida técnica periódica y
conservación de consultas detalladas. Formato visual en placa pendiente;
no se ha cargado firmware durante este cambio.

### Hito 2E.1 — Adquisición eléctrica EC — 2026-10-27

Objetivo: medir impedancia aparente por J1 Yx-1/Yx-2 con resistencias externas,
en las rutas HIGH/RFB 100 ohm (`0x81`) y LOW/RFB 1 kohm (`0x82`).

Cambios: `CN0349Manager` incorpora `cn0349 ec` con selección manual de ruta y
calibración compatible, adquisición asíncrona, resultado EC independiente de
RTD/temperatura, errores y metadatos completos. MuxApp verifica digitalmente
la máscara; ImpedanceApp reutiliza su único motor DFT y standby. Se aplican los
GF 2B existentes sin modificarlos, con coincidencia de RFB/configuración.
`result` muestra `EC PROBE - EXPERIMENTAL / NOT ABSOLUTELY CALIBRATED` y
Z aparente, nunca conductividad. [Topología y R2](docs/ec-route.md),
[API](src/cn0349/README.md), [matriz física](test/hito2e1.md).

```text
cn0349 ec range list
cn0349 ec range set HIGH
cn0349 ec cal list
cn0349 ec cal set CAL_HIGH_100R
cn0349 ec config
cn0349 ec start
cn0349 ec status
cn0349 ec result
cn0349 ec stop
```

Para LOW seleccionar `cn0349 ec range set LOW` y luego una calibración LOW.
La selección de ruta no cambia silenciosamente la calibración; `start` informa
incompatibilidad hasta elegirla. No se sustrae R2, ni se compensa temperatura,
ni se convierten ohmios en unidades de conductividad.

Pruebas: firmware ESP32-S3 compilado SUCCESS (RAM 28592 bytes, Flash 418514
bytes). Suite host `test\host_cn0349\run.cmd`: siete grupos PASS, incluyendo
rutas, compatibilidad, readback y regresiones RTD/temperatura. La matriz física
de 40 adquisiciones está [completa](test/hito2e1.md). La figura oficial CN-0349 es un esquema
simplificado; el netlist de la placa no está en este repositorio y se solicita
verificar la topología de R2 antes de interpretar diferencias. Estado:
adquisición EC implementada y comprobada por software; caracterización
eléctrica experimental completada, exactitud absoluta pendiente.

### Resultados fisicos EC - 2026-10-27

Las 40 adquisiciones válidas A–H se recogen en [la tabla del hito 2E.1](test/hito2e1.md).

## 2026-10-27 — Hitos 2E.2 y 2E.3: sonda real y conductividad estimada

**Objetivo:** medir la impedancia aparente de una sonda EC real y ofrecer una
estimación explícitamente no calibrada en µS/cm y mS/cm. **Cambios:** el comando
`cn0349 conductivity` reutiliza la adquisición EC asíncrona y congela su
snapshot DFT junto al modelo de conversión. El módulo puro `Conductivity`
calcula en modo RAW con K nominal configurable de 1 cm⁻¹. R2 nominal es 100 Ω,
pero su resta experimental se mantiene deshabilitada hasta verificar la
topología equivalente de la placa. No se han modificado los GF, Pt100 ni la
selección explícita de ruta/calibración.

**Pruebas:** suite host con ocho grupos PASS, incluidos cálculo matemático,
modelo RAW, errores, snapshot, standby y regresiones RTD/temperatura.
Compilación ESP32-S3 `esp32-s3-devkitc-1-n32r8v` SUCCESS. **Estado:**
funcionalidad software lista; la prueba física con la sonda y líquidos sigue
pendiente. El [protocolo experimental](test/hito2e2.md) cubre agua de baja
mineralización, grifo y una pequeña adición de sal. El hito 2E.4 queda para
patrones, calibración de K y validación metrológica.
