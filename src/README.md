# Aplicación ESP32-S3

`main.cpp` inicializa Serial y Wire, crea una instancia de los drivers AD5934 y
ADG715, y conecta ImpedanceApp, MuxApp y CN0349Manager al terminal. El Manager
reutiliza ImpedanceApp como único motor DFT. El ciclo llama a
`Terminal::update()` y `CN0349Manager::update()` sin esperas de entrada
ni `delay()`.

`terminal/`, `impedance/`, `mux/` y `cn0349/` están implementados en hitos 1A–2A.
ImpedanceApp coordina adquisiciones DFT de punto único y barrido; MuxApp aplica
presets autorizados y verifica el interlock; CN0349Manager expone API tipada y
comandos de alto nivel. No hay cálculo de impedancia, calibración, conductividad,
Pt100 ni BLE.

## Hito 2C.1

`cn0349/CN0349Impedance.cpp` añade medida aparente Pt100 a la misma instancia
Manager, sobre el motor DFT de `impedance/` y la tabla de rutas de `mux/`.
`main.cpp` y Terminal no necesitan lógica hardware adicional. El Manager acepta
comandos `cn0349 impedance ...`; versión 0.9.0. Ver [arquitectura](../docs/architecture.md).

## Hito 2D — 2026-10-27

`temperature/Pt100.h` convierte resistencia a Celsius mediante IEC 60751 sin
dependencias hardware. `cn0349/CN0349Temperature.cpp` coordina su uso sobre el
resultado asíncrono de impedancia y conserva su procedencia por valor. El parser
del Manager añade `cn0349 temperature ...`; no requiere nuevos módulos Terminal
ni instancias en main. No hay offsets ni compensación de cables. Se conserva el
identificador de firmware existente. [Uso, pruebas y límites](../test/hito2d.md).

## Preparación automática al arrancar — 2026-10-27

Después de inicializar el bus y reportar comunicación, `main.cpp` llama por
defecto a `CN0349Manager::prepareMeasurements()`. El Manager ejecuta recover e
inicia la calibración de los cuatro presets a 2000 Hz. `loop` sigue atendiendo
terminal, Manager y mux mientras avanza la adquisición. El mensaje de sistema
preparado se emite desde Manager al terminar correctamente, no desde setup.
Si falla I2C, el terminal permanece disponible para diagnóstico y no se inicia
la preparación. Ver [API](cn0349/README.md).

## Hito 2E.1 — 2026-10-27

`cn0349/CN0349Ec.cpp` añade adquisición EC HIGH/LOW con el Manager existente;
`impedance/` conserva la única máquina asíncrona DFT y `mux/` valida las dos
máscaras EC. No hay cambios en main ni drivers. El resultado EC queda separado
del RTD y temperatura, con metadatos completos para futura exportación BLE.
[Arquitectura](../docs/architecture.md), [protocolo](../test/hito2e1.md).

## Hitos 2E.2 y 2E.3 — 2026-10-27

`cn0349/` añade comandos `conductivity` sobre su adquisición EC existente y
`conductivity/` calcula la estimación en una función pura, sin acceder al bus.
El resultado une DFT, configuración congelada y conversión RAW; el modelo
serie R2 queda deshabilitado. `main.cpp`, RTD y temperatura mantienen sus
interfaces. La futura BLE podrá transportar el snapshot sin recalcularlo.
[Arquitectura](../docs/architecture.md), [módulo](conductivity/README.md),
[protocolo](../test/hito2e2.md).
