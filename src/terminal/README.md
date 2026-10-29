# Terminal operativo — Hito 2F

## Hito 2G

`setup ec model raw|series|ad|analog_devices` selecciona las tres ramas.
`cal status` añade AD LOW/HIGH. Las medidas muestran RAW, SERIES o AD; GF, NOS,
NX y admitancias permanecen en el snapshot diagnóstico.

`cal show` ofrece bajo demanda los cuatro Gain Factors legacy con ID y los dos
rangos AD con NL, NH, GF en S/code, NOS, ID y validez. Es una consulta de RAM:
no inicia calibración, no accede al bus I²C y no altera resultados publicados.

Propósito: interpretar comandos, validar argumentos, llamar a CN0349Manager y
formatear respuestas compactas. Constructor `Terminal(Stream&, CN0349Manager&)`;
`begin` imprime cabecera; `update` procesa entrada y publica cambios/resultados.
No conoce Wire, drivers ni detalles de secuenciación de sensores.

`setup compensation on|off` activa o desactiva la compensación, y
`setup alpha <valor>` establece el coeficiente en %/°C. El controlador valida
el valor y exige PT100 + EC cuando está activa. `setup show` muestra ambos;
cada ciclo muestra EC25 en µS/cm o `unavailable` si está activa y no es válida.

Buffer fijo de 96 bytes y presupuesto 32 bytes/update, CRLF, tabuladores,
backspace y normalización de mayúsculas/espacios. Líneas desbordadas o con
caracteres no ASCII se descartan completas y generan ERR INVALID_ARGUMENT.
Números enteros positivos sin signo, overflow y argumentos sobrantes se
rechazan. K requiere valor finito y positivo; sensores duplicados se rechazan.

Comandos: help, status; setup show/interval, setup ec range high|low,
setup ec model raw|series, setup ec k; cal status/show/run; measure show/set/once/
start/stop. `measure set` admite PT100, EC, PT100 EC y EC PT100. `series` se
rechaza porque no está verificada la topología. [Sintaxis y formatos](../../docs/architecture.md).

El controlador decide Busy/NotCalibrated/Hardware. El terminal continúa
atendiendo consultas y stop durante adquisición. Se mantiene un anuncio de
calibración por calibrationRunId y un reconocimiento de stop pendiente hasta
que el hardware termina. Solo se imprimen ciclos completos, con errores por
sensor; no se imprime OK si alguno falla. Los cancelados se consumen y omiten.
No se imprimen DFT/GF/REAL/IMAG en operación normal; siguen en snapshots.

Dependencias: Arduino Stream, Config y API de CN0349Manager. Se retiran tabla
de módulos, escáner I²C, info/clear y los tres parsers experimentales. El terminal
es actualmente el consumidor único de takeMeasurementResult; futura BLE debe
compartir un dispatcher según el contrato de arquitectura.
