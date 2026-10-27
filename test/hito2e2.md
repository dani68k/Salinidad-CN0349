# Hitos 2E.2 y 2E.3 — Sonda EC real y conductividad estimada

## Alcance y configuración

La sonda EC de constante de celda nominal **K = 1 cm⁻¹** se conecta a J1 Yx-1/Yx-2 con el equipo apagado. El firmware registra impedancia aparente y calcula una **conductividad estimada, no calibrada**, mediante `1e6 × K / |Z aparente|` en µS/cm. El modo activo es RAW: no resta R2 de 100 Ω. La fase instrumental y la naturaleza compleja de la impedancia impiden tratar esta cifra como una conductividad absoluta validada. El hito 2E.4 queda reservado para soluciones patrón y validación metrológica.

Parámetros iniciales: **2000 Hz**, MCLK externo **1 MHz**, Range 1 (**2 Vpp nominales**), PGA ×1 y settling de 10 ciclos ×1. La salida nominal del AD5934 **no garantiza** esa tensión en los electrodos: el acoplamiento, los amplificadores y la impedancia de la sonda alteran la señal real. No hay detección fiable de saturación analógica, autorange ni barrido.

| Ruta | Máscara | RFB | Calibraciones compatibles |
| --- | --- | ---: | --- |
| HIGH | `0x81` | 100 Ω | `CAL_HIGH_100R`, `CAL_HIGH_1K` |
| LOW | `0x82` | 1000 Ω | `CAL_LOW_1K`, `CAL_LOW_10K` |

Seleccionar ruta y calibración de forma explícita y comprobar `cn0349 ec config` y `cn0349 conductivity config` antes de cada serie. `cn0349 conductivity start` reutiliza la adquisición EC no bloqueante; `cn0349 conductivity status` y `cn0349 conductivity result` permiten revisar estado, configuración congelada, DFT y estimación. `cn0349 conductivity stop` cancela una adquisición activa. `cn0349 ec start` sigue disponible cuando solo interesa la impedancia. Si `start` rechaza una combinación o la muestra falla, corregir la configuración o la causa del error antes de repetirla; no contar un resultado anterior como una adquisición nueva.

## Caracterización cualitativa 2E.2/2E.3

Estas muestras **no son patrones de conductividad** y no tienen un valor asignado en µS/cm. Usar recipientes limpios y seguros; no utilizar líquidos peligrosos ni incompatibles con la sonda. Comenzar por las muestras menos salinas y añadir la sal en pequeñas cantidades.

| ID | Muestra | Preparación cualitativa |
| --- | --- | --- |
| A | Agua de baja mineralización | Sin adición de sal |
| B | Agua del grifo | Sin adición de sal |
| C | Agua con sal | Anotar origen del agua y cantidad de sal añadida; datos de esta sesión pendientes |
| D (opcional, 2E.4) | Agua del grifo con más sal | Aumentar la cantidad respecto de C, sin concentración excesiva |

Para cada muestra:

1. Enjuagar la sonda con agua de baja mineralización; evitar arrastrar líquido de la muestra anterior.
2. Sumergir los electrodos a la misma profundidad, sin burbujas y sin tocar paredes o fondo. Mantener el mismo recipiente o una geometría equivalente.
3. Medir la temperatura de la muestra con el multímetro y esperar a que la lectura se estabilice. Anotar también la duración aproximada de esta espera.
4. Comprobar con `cn0349 ec config` la ruta, RFB y calibración elegidos. Ejecutar cinco veces `cn0349 conductivity start`, esperando el fin de cada adquisición y consultando `cn0349 conductivity result`. Registrar solo resultados nuevos `COMPLETE / VALID / NONE` con standby verificado; conservar aparte el diagnóstico de cualquier fallo mientras se resuelve.
5. Registrar la estimación en µS/cm y mS/cm junto al DFT y Z aparente. Si la señal DFT es muy pequeña, el estado informa un error o hay sospecha de saturación, detener la serie y seleccionar **manualmente** otra combinación compatible. Registrar el cambio como otra serie; no asumir que HIGH/LOW designan directamente una clase de conductividad.
6. Cuando sea eléctricamente adecuado, repetir cinco adquisiciones con la otra ruta o calibración compatible. Mantener temperatura, inmersión y tiempo de espera comparables. Anotar si aparece deriva entre la primera y quinta lectura.

## Registro de resultados

Completar una fila por serie de cinco adquisiciones válidas. Conservar por cada lectura REAL, IMAG, magnitud DFT, fase DFT, fase corregida, impedancia aparente, conductividad estimada, estado, error y tiempo; la ruta, máscara, RFB, calibración, ID, GF, frecuencia efectiva, excitación, PGA, modelo RAW, K nominal, R2 nominal, temperatura externa, fecha o identificador de sesión y condiciones de inmersión pertenecen al encabezado de esa serie. Los valores se copian de `conductivity result` y no se tratan como conductividad calibrada. El snapshot actual vive en RAM y conserva la configuración de la adquisición; estas tablas permiten archivar las lecturas para la futura WebApp BLE, sin implementar aún almacenamiento persistente.

| Sesión | Muestra / descripción | Temperatura multímetro (°C) | Ruta / máscara / RFB | Calibración / ID / GF | Frecuencia efectiva (Hz) | Observaciones |
| --- | --- | ---: | --- | --- | ---: | --- |
| Pendiente | A / agua de baja mineralización | — | — | — | — | — |
| Sesión grifo 1 | B / agua del grifo | Pendiente | LOW `0x82` / 1000 Ω | `CAL_LOW_1K` / ID 3 / GF 0.000000121290 | 1999.996582 | K nominal 1 cm⁻¹; RAW; R2 nominal 100 Ω; inmersión y temperatura sin documentar |
| Sesión sal 1 | C / agua del grifo con sal; cantidad sin documentar | Pendiente | LOW `0x82` / 1000 Ω | `CAL_LOW_1K` / ID 3 / GF 0.000000121290 | 1999.996582 | K nominal 1 cm⁻¹; RAW; R2 nominal 100 Ω; inmersión y temperatura sin documentar |
| Pendiente | D / agua del grifo con más sal | — | — | — | — | — |

| Muestra / serie | Lectura | REAL | IMAG | Magnitud DFT | Fase DFT (°) | Fase corregida (°) | Z aparente (Ω) | EC estimada (µS/cm) | Estado / error | Tiempo (ms) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| B / grifo 1 | 1 | -2131 | -3566 | 4154.216797 | -120.862045 | -6.322502 | 1984.658813 | 503.864929 | COMPLETE / NONE | 229 |
| C / sal 1 | 1 | -13563 | -14768 | 20051.154297 | -132.564514 | -18.024971 | 411.183441 | 2432.004639 | COMPLETE / NONE | 223 |

Las primeras adquisiciones B1 y C1 fueron `VALID`, sin error de hardware y con standby verificado. Con la misma ruta y calibración, la muestra salada muestra menor Z aparente (**411.183441 Ω** frente a **1984.658813 Ω**) y mayor conductividad RAW estimada (**2432.004639** frente a **503.864929 µS/cm**). La fase corregida también cambia de **−6.322502°** a **−18.024971°**; no puede atribuirse exclusivamente a los electrodos. No constan temperatura, cantidad de sal ni repeticiones suficientes para evaluar repetibilidad o exactitud.

Al cerrar cada serie, calcular media y desviación estándar muestral de Z, coeficiente de variación (`100 × desviación/media`), mínimo, máximo y variación de fase corregida; registrar las condiciones de estabilidad. Comparar si Z tiende a disminuir al aumentar la sal, la dispersión y deriva de cada muestra, las diferencias entre HIGH y LOW y el efecto de elegir otro GF con la misma ruta. Anotar combinaciones que produzcan señal DFT pequeña, error o posible saturación, sin inventar un umbral de saturación no validado. Una fase cambiante puede incluir polarización de electrodos, deriva térmica y efectos de la electrónica; la corrección instrumental de fase aún no valida la fase física de la celda.

### Comparación de rutas y repetibilidad

Usar solo combinaciones con señal utilizable: HIGH + `CAL_HIGH_100R`, HIGH + `CAL_HIGH_1K`, LOW + `CAL_LOW_1K` y LOW + `CAL_LOW_10K`. No asumir que mayor magnitud DFT significa mayor exactitud.

| Muestra / sesión | Ruta y calibración | n | Media Z (Ω) | s muestral (Ω) | CV (%) | Z mín.–máx. (Ω) | Fase mín.–máx. (°) | Observaciones |
| --- | --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| Pendiente | — | — | — | — | — | — | — | — |

### Estabilidad temporal (reservado para 2E.4)

Con la sonda inmóvil en una muestra, hacer **diez adquisiciones espaciadas manualmente**. Registrar el tiempo relativo de cada una y mantener ruta, calibración, profundidad y temperatura lo más constantes posible. La secuencia sirve para detectar deriva; no requiere un temporizador nuevo en el firmware.

| Muestra / sesión | Ruta y calibración | Lectura 1–10 | Tiempo relativo (s) | Temperatura (°C) | Magnitud DFT | Z aparente (Ω) | Fase corregida (°) | Observación |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Pendiente | — | — | — | — | — | — | — | — |

## Soluciones patrón (reservadas para 2E.4)

En el futuro 2E.4, si se dispone de soluciones comerciales, preferir **84 µS/cm**, **1413 µS/cm** y **12,88 mS/cm**. Registrar fabricante, identificación, conductividad nominal, temperatura de referencia, tolerancia declarada y vigencia. Medir la temperatura real con el multímetro y repetir cinco adquisiciones con inmersión, espera y configuración documentadas. No comparar directamente una conductividad certificada a 25 °C con una medida a otra temperatura sin considerar la dependencia térmica del patrón. No ajustar K ni los GF con las lecturas actuales.

| Patrón / lote / sesión | Nominal y unidad | T ref. (°C) | T real (°C) | Tolerancia | Ruta / calibración | n | Media Z (Ω) | s (Ω) | Fase media (°) | Observaciones |
| --- | --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | --- |
| Pendiente | — | — | — | — | — | — | — | — | — | — |

Comparar la **tendencia eléctrica** y la dispersión entre rutas y calibraciones; la conductividad estimada por RAW no equivale a una conductividad calibrada ni permite deducir una constante de celda efectiva o una corrección automática a partir de este hito. Dejar explícito cuando no se disponga de patrones; 2E.4 no se declara ejecutado por preparar este protocolo.

## Limitaciones observadas

| Muestra / configuración | Señal DFT pequeña, error o sospecha de saturación | Deriva / fase / temperatura | Acción y resultado |
| --- | --- | --- | --- |
| Pendiente | — | — | — |

## Estado de validación

El firmware reutiliza los comandos EC existentes y conserva los snapshots por adquisición. Las pruebas host verifican compatibilidad de calibración, errores, standby, configuración conservada, cálculo RAW y regresión de Pt100/temperatura; **no simulan** la sonda ni el líquido. La compilación del ESP32-S3 finaliza correctamente; las primeras adquisiciones físicas con la sonda real siguen pendientes. La exactitud absoluta, la constante de celda efectiva, la sustracción de R2 y la compensación térmica quedan pendientes de validación metrológica en 2E.4.
