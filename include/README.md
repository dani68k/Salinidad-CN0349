# Configuración compartida

`config.h` contiene constantes `constexpr` en el espacio `Config`: identidad,
versión e hito, baudios serie, pines y frecuencia I²C, timeout, direcciones
AD5934/ADG715, rango de escaneo y límites del terminal. No contiene estado
mutable ni inicializa periféricos.
`main.cpp` aplica la configuración y `Terminal` la muestra y utiliza sus límites.
Si se cambia `SERIAL_BAUD`, sincronizar también `monitor_speed` en `platformio.ini`.
`ad5934_math.h` contiene conversiones matemáticas puras para palabra DDS,
decodificación DFT con signo, magnitud, fase, límites/acumulación de barrido y
frecuencia efectiva, compartidas por el driver, ImpedanceApp y pruebas host.
`impedance_diagnostics.h` define códigos tipados y el registro RAM del último fallo: subsistema, fase, timestamp millis, detalle, recuperabilidad y estado de recuperación. Se comparte entre ImpedanceApp y las pruebas Unity; los contadores viven en ImpedanceApp.

## Hito 2C.1

`config.h` identifica firmware 0.9.0 / hito 2C.1. Se conservan bus, pines, baudios
y timeout. La matemática específica Pt100 vive en `src/cn0349/ImpedanceMath.h`.

## Hito 2E.3

`EC_NOMINAL_CELL_CONSTANT_PER_CM = 1.0` y `EC_NOMINAL_R2_OHMS = 100.0`
identifican parámetros nominales del modelo EC. Son constantes de compilación,
no parámetros calibrados ni correcciones automáticas; el módulo
`src/conductivity/` las recibe mediante el Manager.
