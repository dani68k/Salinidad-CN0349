# ATLANTES CN0349

ATLANTES es un instrumento de análisis de agua basado en ESP32-S3 y la placa
CN0349. Adquiere temperatura con una Pt100 y estima la conductividad eléctrica
(EC) de una celda mediante el convertidor de impedancia AD5934. Un terminal
serie y una WebApp Bluetooth Low Energy (BLE) comparten el mismo controlador.

El proyecto cubre adquisición, calibración electrónica, configuración y
visualización. Los resultados dependen del circuito y de la celda: una
calibración electrónica no sustituye una calibración metrológica con patrones.
Los resultados de banco y sus límites se recogen en [VALIDACION.md](test/VALIDACION.md).

## Acceso a la documentación

- **Proyecto:** este README describe el alcance, la arquitectura y el uso.
- **Validación:** [evidencias y resultados históricos](test/VALIDACION.md).
- **Referencia técnica:** [abrir Doxygen](Doxygen.html), con interfaces y
  diagramas. También puede abrirse directamente
  [el índice generado](build/doxygen/html/index.html).

El HTML de Doxygen se genera localmente y no se guarda en Git. Si aún no existe,
crear la carpeta `build/doxygen` y ejecutar `doxygen Doxyfile` desde la raíz;
`DOT_PATH` debe apuntar a la instalación local de Graphviz.

## Hardware y configuración

| Elemento | Configuración actual |
|---|---|
| Controlador | ESP32-S3, Arduino y PlatformIO |
| Placa de medida | CN0349, AD5934 y multiplexor ADG715 |
| Bus I²C | SDA GPIO4, SCL GPIO5; 100 kHz; timeout 25 ms |
| Direcciones I²C | AD5934: 0x0D; ADG715: 0x48 |
| Puerto serie | 115200 baudios |
| Perfil AD5934 | MCLK externo 1 MHz; 2 kHz; Range 1, 2 Vpp nominal; PGA ×1; 10 ciclos |
| Adquisición | Frecuencia única; incremento y número de incrementos cero |
| Valores EC nominales | K inicial 1 cm⁻¹; R2 100 Ω |

Los pines y parámetros de compilación se definen en
[config.h](include/config.h). El código identifica el firmware como **1.0.0 /
3B**; ese identificador no debe confundirse con el último hito funcional del
desarrollo, 3I. La frecuencia DDS efectiva está cuantizada y el ACK I²C no
confirma el funcionamiento físico del reloj.

La CN0349 utiliza las rutas siguientes:

| Función | Máscara ADG715 | RFB |
|---|---:|---:|
| Pt100 | 0x41 | 100 Ω |
| EC HIGH | 0x81 | 100 Ω |
| EC LOW | 0x82 | 1 kΩ |
| Calibraciones HIGH | 0x09 / 0x11 | 100 Ω |
| Calibraciones LOW | 0x12 / 0x22 | 1 kΩ |

Las referencias electrónicas son HIGH 100 Ω y 1 kΩ, y LOW 1 kΩ y 10 kΩ.
El readback del multiplexor confirma el registro digital, no la continuidad
analógica. Confirmar conexiones y orientación de la placa antes de cablear
Pt100 o celda.

## Arquitectura

```text
Terminal serie ─┐
               ├── CN0349Manager ── ImpedanceApp ── AD5934
WebApp ─ BLE ──┘        │                └───────── MuxApp ── ADG715
                        ├── conversión Pt100
                        └── conversión de conductividad
```

`src/main.cpp` inicializa Wire, el controlador, el terminal y BLE. El
`loop()` avanza las operaciones de forma cooperativa. `CN0349Manager`
coordina configuración, calibración y ciclos; `ImpedanceApp` gestiona la
adquisición DFT; `MuxApp` controla las rutas seguras. Los módulos Pt100 y
Conductivity realizan conversiones; los drivers de `lib/` acceden al hardware.
Los callbacks BLE encolan órdenes para su ejecución en el loop.

La documentación específica de interfaces y módulos permanece junto al código
en `src/`, `lib/` e `include/`. La [WebApp](web/README.md) tiene su propia
documentación.

## Funcionamiento

### Arranque, calibración y medida

Al arrancar se carga el Setup persistido si es válido, se comprueba el hardware
y se inicia la calibración electrónica. Se realizan cinco adquisiciones por
cada una de las cuatro referencias. Los resultados incluyen magnitud, factor de
calibración, fase, configuración y validez. Una referencia fallida no se
sustituye por otra; la disponibilidad de cada medida depende de las
calibraciones compatibles con su ruta.

Se puede medir Pt100, EC o ambas, una vez o de forma continua. En cada ciclo
Pt100 se adquiere antes de EC, usando una sola máquina de adquisición. El
controlador conserva un snapshot del ciclo con su configuración y resultados.
La parada cancela la adquisición y procura llevar el AD5934 a standby.

El intervalo inicial es 1000 ms; el siguiente ciclo continuo comienza cuando
se cumple el intervalo y no hay una adquisición activa. Las transacciones I²C
son síncronas y están acotadas por el timeout de Wire.

### Conductividad y temperatura

La magnitud DFT y la calibración permiten calcular una impedancia aparente.
El controlador ofrece tres modelos EC:

- **RAW:** estima conductividad a partir de K y la impedancia aparente.
- **SERIES:** modelo experimental que resta la R2 nominal a la impedancia.
- **AD:** utiliza la calibración de dos puntos y offset de la CN0349, con
  corrección R2 en el cálculo de admitancia.

K es configurable; su valor inicial es nominal. La topología y transferencia
analógica real, el comportamiento complejo de la celda y la exactitud
metrológica requieren caracterización. La Pt100 se convierte con IEC 60751;
no se aplica compensación de cables.

La compensación térmica es opcional, está desactivada por defecto y exige
seleccionar Pt100 + EC. Usa ambos resultados del mismo ciclo:

`EC25 = EC / (1 + alpha / 100 × (T − 25))`

La referencia es 25 °C y alpha se configura entre 0,00 y 5,00 %/°C, por
defecto 2,00 %/°C. Si una lectura falla o el denominador no es válido, EC25 no
está disponible. La EC medida se conserva y el cálculo no modifica sus modelos.

### Setup y diagnóstico EC

Setup permite configurar sensores, intervalo, rango EC, modelo, K y compensación
térmica. APPLY aplica el conjunto validado y CANCEL recupera el último
confirmado. La configuración se guarda en Preferences/NVS, namespace `cn0349`,
como blob versionado; se admiten formatos anteriores. Si falla la escritura,
el cambio permanece en RAM y se informa del fallo. Las calibraciones y los
resultados no se guardan en NVS.

Advanced Setup muestra el perfil AD5934 configurado como **READ ONLY**. No es
una lectura actual de registros ni permite editar el perfil.

El diagnóstico EC compara la magnitud DFT con las dos referencias compatibles
del rango seleccionado. Indica dentro del intervalo, cerca del límite superior
(último 15 %), por debajo, por encima o calibración inválida. La WebApp usa
blanco, naranja o rojo para la EC medida. LOW por encima del intervalo sugiere
HIGH; HIGH por debajo sugiere probar LOW sin garantizar que sea adecuado.
El diagnóstico no detecta saturación física ni cambia de rango automáticamente.

## Uso

### Compilación y terminal

Abrir el proyecto con PlatformIO en VS Code, compilar y cargar el entorno
`esp32-s3-devkitc-1-n32r8v`. También puede usarse:

```text
pio run -e esp32-s3-devkitc-1-n32r8v
pio run -e esp32-s3-devkitc-1-n32r8v -t upload
pio device monitor -b 115200
```

Comandos actuales:

```text
help
status
setup show
setup interval <ms>
setup ec range high|low
setup ec model raw|series|ad
setup ec k <K>
setup compensation on|off
setup alpha <0..5>
cal status
cal show
cal run
measure show
measure set PT100|EC|PT100 EC
measure once
measure start
measure stop
```

`setup alpha` admite dos decimales. Con compensación activa no se acepta
seleccionar un único sensor. La configuración no puede cambiarse mientras el
instrumento está ocupado. El terminal también informa de EC25 cuando está activa.

### WebApp

Abrir [web/index.html](web/index.html) en un navegador compatible con Web
Bluetooth. Si `file://` no ofrece la API, servir `web/` desde `localhost`.
Conectar al dispositivo `CN0349`; las vistas son Measurement, Setup y
Calibration.

Measurement muestra la última Pt100 y EC, EC25 cuando está activa y el historial
de hasta 50 ciclos. El gráfico usa un eje para temperatura y otro compartido
para EC y EC25, con escalas AUTO/FIXED y botón CLEAR. Las unidades EC y los
límites gráficos son preferencias de visualización locales.

Al desconectarse el último central BLE se solicita la parada de la medición
activa y se descartan órdenes BLE de control pendientes. Terminal puede iniciar
medidas posteriormente sin central conectado.

## BLE

El servicio ATLANTES es `8d7b0001-6f7a-4f31-9c4d-2b6e6a7f3a10`. Todas las
características siguientes comparten el sufijo
`-6f7a-4f31-9c4d-2b6e6a7f3a10`:

| Prefijo UUID | Característica | Propiedades | Función |
|---|---|---|---|
| 8d7b0002 | STATUS | READ, NOTIFY | Estado, calibraciones y ocupación |
| 8d7b0003 | SETUP | READ, NOTIFY | Setup básico |
| 8d7b0004 | SETUP_COMMAND | WRITE | Aplicar Setup básico |
| 8d7b0005 | SETUP_RESULT | READ, NOTIFY | Resultado correlacionado |
| 8d7b0006 | CALIBRATION | READ | Referencias y calibraciones |
| 8d7b0007 | MEASUREMENT | READ, NOTIFY | Ciclo Pt100 y EC |
| 8d7b0008 | CONTROL_COMMAND | WRITE | Calibración, sensores y medida |
| 8d7b0009 | CONTROL_RESULT | READ, NOTIFY | Resultado de control |
| 8d7b000a | ADVANCED | READ, NOTIFY | Perfil AD5934 de solo lectura |
| 8d7b000b | EC_DIAGNOSTIC | READ, NOTIFY | Diagnóstico EC por ciclo |
| 8d7b000c | SETUP_EXT | READ, NOTIFY | Setup con compensación |
| 8d7b000d | EC25 | READ, NOTIFY | Compensación y EC25 por ciclo |
| 8d7b000e | SETUP_EXT_COMMAND | WRITE | Aplicar Setup ampliado |

Los paquetes binarios usan versión 1 y little-endian; `cycleId` asocia
MEASUREMENT, EC_DIAGNOSTIC y EC25. El formato, offsets y códigos se detallan
en [src/ble/README.md](src/ble/README.md). No se implementan bonding ni
reconexión automática.

## Validación y límites

El desarrollo recoge compilaciones y pruebas simuladas históricas, así como
ensayos físicos de rutas y cargas. Las suites automatizadas históricas se han
retirado durante la consolidación; sus resultados y el alcance de cada prueba
se resumen en [test/VALIDACION.md](test/VALIDACION.md).

El cierre funcional de los hitos no acredita exactitud absoluta. La evidencia
disponible no sustituye ensayos con patrones de conductividad, una referencia
independiente de temperatura ni la caracterización de la placa y la sonda.
La corrección de R2, la compensación térmica y el uso de K nominal deben
interpretarse dentro de esas limitaciones.

## Resumen del desarrollo

| Etapa | Resultado principal |
|---|---|
| 1A–1C | Infraestructura serie/I²C, drivers AD5934 y ADG715 y control de rutas |
| 1D–1E | Adquisición DFT, exploración de barridos, diagnóstico y recuperación |
| 2A–2B | Controlador CN0349 y calibración electrónica por referencias |
| 2C–2D | Ruta Pt100, impedancia aparente y conversión de temperatura |
| 2E–2G | Ruta EC, conductividad estimada y modelos RAW, SERIES y AD |
| 3A–3D | Servicio BLE, Setup, calibración, medidas y gráfica en WebApp |
| 3E–3F | Advanced Setup de lectura y persistencia del Setup |
| 3G | Edición avanzada explorada y retirada; el perfil permanece de lectura |
| 3H | Diagnóstico EC frente al intervalo de calibración |
| 3I | Compensación de EC a 25 °C y representación EC/EC25 |

Los hitos describen evolución del proyecto; no todos los comandos experimentales
de versiones anteriores forman parte del firmware actual.

## Repositorio y referencias

- `src/`: controlador, adquisición, sensores, terminal y BLE.
- `lib/`: drivers AD5934 y ADG715.
- `include/`: configuración y utilidades compartidas.
- `web/`: interfaz HTML/CSS/JavaScript con Web Bluetooth.
- `test/VALIDACION.md`: evidencia histórica consolidada.
- [AD5934, datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ad5934.pdf).
- [ADG715, datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/adg714_715.pdf).
- [CN-0349, diseño de referencia](https://www.analog.com/media/en/reference-design-documentation/reference-designs/cn0349.pdf).
