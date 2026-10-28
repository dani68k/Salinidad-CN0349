# Hito 2F — configuración vigente (2026-10-27)

## Hito 2G

`MILESTONE` pasa a `2G`. K=1 cm⁻¹ y R2=100 Ω siguen siendo nominales. Las
estructuras AD viven en Conductivity/CN0349Manager, no en Config.

`config.h` contiene versión 1.0.0/Hito 2F, pines I²C, baudrate, timeout 25 ms,
K/R2 nominales y límites del buffer/servicio del terminal. Sin NVS ni estado
mutable global. El setup operativo reside únicamente en CN0349Manager.
`ad5934_math.h` conserva fórmulas DDS/control y `impedance_diagnostics.h`
conserva errores tipados y registros de recuperación usados por ImpedanceApp.
La API y los errores de instrumento están en CN0349Manager.h. Estos headers
no realizan I²C ni imprimen; main inicializa dependencias. Las notas posteriores
son historial de hitos anteriores.

---

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
