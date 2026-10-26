# Terminal de diagnóstico

`Terminal.h` y `Terminal.cpp` implementan un receptor de líneas y un escáner I²C
independientes de los futuros drivers. Las respuestas usan texto ASCII para
facilitar su lectura en monitores serie.

## Interfaz

- `Terminal(Stream&, TwoWire&)`: inyecta entrada/salida y bus; no inicializa hardware.
- `begin(bool i2cReady)`: reinicia entrada e historial y registra el resultado de inicialización.
- `update()`: consume hasta 32 bytes disponibles y realiza como máximo un sondeo.
- `printInfo()`: imprime identidad y parámetros configurados, sin acceder al bus.
- `attachModule(context, handler, busy, help)`: registra un módulo mediante
  callbacks, sin que el terminal dependa de su driver. Hay dos espacios
  estáticos para ImpedanceApp y MuxApp; un registro adicional se rechaza.

Se usa `Serial` en producción. Los handlers y descripciones están en `commands_`;
para ampliar comandos se añade un método y una entrada en esa tabla. `help` y
el despacho comparten la tabla. Después consulta el callback de aplicación con
la línea normalizada y el estado de ocupación de I²C. `help` incluye la ayuda del
módulo. No se reserva memoria dinámica en esta clase.

## Entrada y errores

Buffer de 96 bytes, incluidos 95 caracteres y NUL. CR, LF y CRLF terminan la
línea; un LF después de CR no vuelve a ejecutarla, incluso entre llamadas.
Se ignoran líneas vacías o con solo espacios, se normalizan mayúsculas y espacios
y se aceptan tabuladores. Retroceso y DEL borran un carácter del buffer; no hay
eco ni edición visual. Caracteres no ASCII o de control no admitidos invalidan
la línea. Al exceder el límite se informa una vez y se descarta todo hasta CR/LF,
incluidos retrocesos posteriores. El siguiente comando vuelve a procesarse.
No se aceptan argumentos extra ni coincidencias por prefijo.

## Comandos y diagnóstico

`help`, `info`, `i2c scan`, `i2c status` y `clear` forman la interfaz inicial.
`clear` emite `ESC[2J ESC[H` sin modificar el historial del diagnóstico.

`i2c scan` inicia un estado cooperativo y rechaza una segunda solicitud mientras
está activo. Cada `update()` sondea la siguiente dirección de `0x08` a `0x77`
mediante `beginTransmission()` y `endTransmission()` con STOP y sin `write()`.
Solo ACK produce `Device found`. NACK de dirección significa sin respuesta;
otros códigos producen un mensaje de error con dirección y código.

Al completar se copian contadores y última transacción/error al historial.
`i2c status` distingue el progreso actual del último escaneo completo, que sigue
disponible durante un nuevo escaneo. Antes del primero informa «ninguno».
La inicialización fallida impide sondear pero permite todos los demás comandos.

Una transacción Wire es síncrona y tiene timeout de 25 ms configurado por `main`;
la transmisión de respuestas serie también puede esperar. No hay bucles de
espera de entrada ni un bucle que recorra todo el bus en una sola llamada.
