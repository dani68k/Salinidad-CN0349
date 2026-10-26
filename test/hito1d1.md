# Validación — Hito 1D.1

## Estado y referencias

El firmware 0.4.1 implementa una adquisición DFT de un punto. La compilación debe
registrarse separada de las pruebas físicas. El perfil usa la referencia externa
de 1 MHz del circuito CN-0349 y no 16 MHz: [datasheet AD5934 Rev. E](https://www.analog.com/media/en/technical-documentation/data-sheets/ad5934.pdf),
[CN-0349 Rev. A](https://www.analog.com/media/en/reference-design-documentation/reference-designs/cn0349.pdf).
La palabra para 2 kHz se calcula con `floor(2000 * 2^27 / 1000000) = 0x041893`.

Las pruebas unitarias puras están en `test/test_ad5934_math/test_main.cpp` y se
ejecutan con `pio test -e native` en un equipo que tenga toolchain C/C++ (gcc/g++).
Cubren conversión DDS, límites de frecuencia, enteros con signo, magnitud en ancho
ampliado, reservados de settling y máscaras/operaciones de control. En el equipo
de desarrollo actual no están disponibles `gcc`/`g++`, por lo que esas pruebas no
se pudieron ejecutar aquí. La compilación ESP32 valida integración, no ejecución
de las pruebas nativas.

## Preparación física

1. Cargar firmware 0.4.1 y conectar el monitor a 115200.
2. Confirmar alimentación y cableado; verificar con osciloscopio o contador que
   MCLK del AD5934 tiene aproximadamente 1 MHz. El ACK I²C no verifica MCLK.
3. Asegurar que el equipo es la EVAL-CN0349-PMDZ con su oscilador. Conectar SDA
   GPIO4, SCL GPIO5 y masa común según el montaje validado.
4. No conectar una celda desconocida para la primera prueba: la ruta seleccionada
   es la resistencia/calibración interna de 1 kΩ (`CAL_LOW_1K`, máscara `0x12`).

## Secuencia terminal

```text
ad5934 ping
adg715 ping
ad5934 measure help
ad5934 measure config
ad5934 measure start
ad5934 measure status
ad5934 measure result
ad5934 status
adg715 read
```

`config` debe mostrar MCLK 1 MHz externo, 2 kHz, DDS `0x041893`, Range 1 / 2 Vpp
nominal, PGA ×1, CAL_LOW_1K, cero incrementos y settling 10 ×1. El inicio debe
configurar/verificar registros, aplicar Initialize, esperar, aplicar Start Sweep
y consultar D1. Solo con D1 válido se leen Real/Imaginary. La salida es código
DFT crudo, no ohmios. Al finalizar, control AD5934 debe estar en standby con
Control Low `0x08`; ADG715 debe permanecer en `0x12`.

Después de `start`, repetir `measure status` hasta que indique `COMPLETE` o
`ERROR`; ejecutar `measure result` solo en `COMPLETE`.

Realizar al menos cinco ejecuciones secuenciales de `ad5934 measure start`,
esperando COMPLETE antes de la siguiente. Guardar para cada una: REAL, IMAG,
magnitud, tiempo, D1/status final y cualquier error. Valorar repetibilidad con
los datos observados; no imponer umbral inventado ni concluir precisión de
impedancia/conductividad.

## Registro de validación física

Primera adquisición completada con firmware 0.4.1. `ad5934 measure config`
mostró el perfil esperado (MCLK externo 1 MHz, 2 kHz, DDS `0x041893`, Range 1,
PGA ×1, CAL_LOW_1K, cero incrementos y settling 10). `ad5934 measure start`
terminó con Status VALID (D1), REAL `-3405`, IMAG `-7513`, magnitud DFT
`8248.587`, elapsed `176 ms` y standby final. `measure status` reportó COMPLETE
sin error y `measure result` repitió la misma muestra. ADG715 status/read
confirmaron el registro digital `0x12` (CAL_LOW_1K), sin error previo. Esto valida
una primera adquisición DFT en el montaje reportado; no es una medición en ohmios
ni comprueba eléctricamente la ruta analógica. Faltan cuatro ejecuciones
secuenciales para completar la serie de cinco y valorar repetibilidad.

## Fallo físico reportado en firmware 0.4.0

El usuario reportó que `ad5934 measure start` confirmó `CAL_LOW_1K = 0x12`, pasó
a configurar los registros y terminó con `timeout esperando D1 de datos DFT`;
no se informó error I²C. No se aportaron REAL/IMAG válidos. Esta ejecución no
valida la adquisición.

Para la siguiente sesión, recoger estas lecturas sin escribir registros:

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

Comprobar también con osciloscopio o contador MCLK=1 MHz directamente en el pin
del AD5934. El driver no detecta por I²C la presencia ni la frecuencia de MCLK.
El timeout sin error de bus no distingue entre reloj ausente y otras causas
internas de adquisición.

La revisión del flujo encontró que firmware 0.4.0 enviaba Initialize tras
configurar, pero omitía la transición explícita de power-down a standby requerida
antes del barrido por la Figura 24 del datasheet. Firmware 0.4.1 añade y verifica
power-down→standby antes de programar los parámetros; esta corrección aún debe
validarse físicamente.

Las lecturas de diagnóstico posteriores al fallo 0.4.0 mostraron Control=`0xB108`, Start
Frequency=`0x041893`, Increment=`0x000000`, Number of Increments=`0x0000`,
Settling=`0x000A`, Status=`0x70` (D1=0) y ADG715=`0x12`. La recuperación dejó el
AD5934 en standby y no se capturó error I²C; no hubo muestra DFT válida. La
compilación de 0.4.1 terminó en `SUCCESS` (7,66 s; RAM 22 040 bytes, Flash
369 606 bytes). La adquisición inicial con 0.4.1 está registrada arriba; quedan
pendientes cuatro repeticiones físicas.

## Errores, cancelación y aceptación

- Probar `ad5934 measure set freq 0`, `62501` y texto no decimal: todos deben
  rechazarse; `1`, `2000` y `62500` son los límites/perfil permitidos.
- Probar settling `512` y texto no decimal: rechazo; `0` y `511` son aceptados.
- Mientras mide, enviar `ad5934 measure status`, `ad5934 measure stop` y una
  orden `adg715 set ...`; status/stop siguen disponibles y el cambio de mux se
  rechaza por ocupado.
- Con montaje controlado sin MCLK (solo si se puede desconectar de forma segura),
  verificar timeout, error, intento de standby y que no se inicia otra medida
  automáticamente. Reconectar MCLK y repetir ping; no usar datos inválidos.
- Inducir fallos I²C solo con montaje controlado; comprobar que no se muestra
  resultado nuevo y que se informa fase/registro/error. No retirar conexiones en
  una ruta energizada sin procedimiento de laboratorio.
- Revisar `ad5934 read16 0x80`, `ad5934 read 0x81`, registros de configuración y
  `adg715 read` después de la adquisición; no deben aparecer escrituras ajenas
  al perfil documentado.

La repetibilidad de cada perfil debe valorarse con varias salidas observadas.
Una magnitud repetible solo valida consistencia DFT en esa configuración, no
precisión metrológica ni una calibración.
