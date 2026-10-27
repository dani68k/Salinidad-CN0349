# Hito 2E.1 — Caracterización eléctrica EC

## Configuración y resultados

Las series A–D usan una resistencia externa de **220 Ω nominales** y las series E–H una de **2,2 kΩ nominales**. La ruta EC del CN0349 incluye **R2 = 100 Ω en serie** con la carga externa. No consta una medida actual de estas resistencias con multímetro. Todas las filas registradas terminaron en `COMPLETE / VALID / NONE`, sin error de hardware y con standby verificado. Frecuencia solicitada: 2000 Hz; efectiva: 1999.996582 Hz; efectiva de calibración: 1999.996460 Hz. MCLK externo: 1 MHz; Range 1; PGA ×1; settling: 10 ciclos.

| Serie | Resistencia nominal | Ruta EC | RFB | Calibración | ID | GF | Muestras |
| --- | ---: | --- | ---: | --- | ---: | ---: | ---: |
| A | 220 Ω | HIGH `0x81` | 100 Ω | `CAL_HIGH_100R` | 1 | 0.000001221432 | 5/5 |
| B | 220 Ω | HIGH `0x81` | 100 Ω | `CAL_HIGH_1K` | 2 | 0.000001164789 | 5/5 |
| C | 220 Ω | LOW `0x82` | 1000 Ω | `CAL_LOW_1K` | 3 | 0.000000121298 | 5/5 |
| D | 220 Ω | LOW `0x82` | 1000 Ω | `CAL_LOW_10K` | 4 | 0.000000119253 | 5/5 |
| E | 2,2 kΩ | HIGH `0x81` | 100 Ω | `CAL_HIGH_100R` | 1 | 0.000001221432 | 5/5 |
| F | 2,2 kΩ | HIGH `0x81` | 100 Ω | `CAL_HIGH_1K` | 2 | 0.000001164789 | 5/5 |
| G | 2,2 kΩ | LOW `0x82` | 1000 Ω | `CAL_LOW_1K` | 3 | 0.000000121298 | 5/5 |
| H | 2,2 kΩ | LOW `0x82` | 1000 Ω | `CAL_LOW_10K` | 4 | 0.000000119253 | 5/5 |

| Medida | REAL | IMAG | Magnitud DFT | Fase corregida (°) | Impedancia aparente (Ω) | Tiempo total (ms) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| A1 | -1102 | -2433 | 2670.934814 | +0.128235 | 306.526031 | 229 |
| A2 | -1101 | -2434 | 2671.433594 | +0.156624 | 306.468781 | 217 |
| A3 | -1101 | -2434 | 2671.433594 | +0.156624 | 306.468781 | 217 |
| A4 | -1101 | -2435 | 2672.344727 | +0.165459 | 306.364319 | 213 |
| A5 | -1101 | -2434 | 2671.433594 | +0.156624 | 306.468781 | 213 |
| B1 | -1101 | -2434 | 2671.433594 | +0.452705 | 321.372192 | 213 |
| B2 | -1101 | -2435 | 2672.344727 | +0.461540 | 321.262634 | 217 |
| B3 | -1101 | -2435 | 2672.344727 | +0.461540 | 321.262634 | 213 |
| B4 | -1101 | -2435 | 2672.344727 | +0.461540 | 321.262634 | 213 |
| B5 | -1101 | -2435 | 2672.344727 | +0.461540 | 321.262634 | 213 |
| C1 | -7801 | -17268 | 18948.335938 | +0.209801 | 435.086731 | 229 |
| C2 | -7799 | -17268 | 18947.511719 | +0.215309 | 435.105682 | 213 |
| C3 | -7800 | -17268 | 18947.923828 | +0.212555 | 435.096191 | 213 |
| C4 | -7799 | -17267 | 18946.601563 | +0.214066 | 435.126556 | 213 |
| C5 | -7801 | -17267 | 18947.423828 | +0.208549 | 435.107666 | 217 |
| D1 | -7800 | -17268 | 18947.923828 | +0.526443 | 442.555023 | 217 |
| D2 | -7798 | -17266 | 18945.277344 | +0.529465 | 442.616882 | 213 |
| D3 | -7800 | -17268 | 18947.923828 | +0.526443 | 442.555023 | 217 |
| D4 | -7799 | -17267 | 18946.601563 | +0.527954 | 442.585937 | 217 |
| D5 | -7800 | -17267 | 18947.011719 | +0.525200 | 442.576324 | 217 |
| E1 | -164 | -351 | 387.423553 | -0.547844 | 2113.219727 | 225 |
| E2 | -164 | -351 | 387.423553 | -0.547844 | 2113.219727 | 213 |
| E3 | -164 | -351 | 387.423553 | -0.547844 | 2113.219727 | 213 |
| E4 | -163 | -352 | 387.908508 | -0.351501 | 2110.577637 | 217 |
| E5 | -163 | -352 | 387.908508 | -0.351501 | 2110.577637 | 217 |
| F1 | -163 | -351 | 387.001282 | -0.117630 | 2218.402344 | 217 |
| F2 | -163 | -352 | 387.908508 | -0.055420 | 2213.214111 | 213 |
| F3 | -163 | -351 | 387.001282 | -0.117630 | 2218.402344 | 218 |
| F4 | -164 | -352 | 388.329742 | -0.189308 | 2210.813232 | 213 |
| F5 | -163 | -352 | 387.908508 | -0.055420 | 2213.214111 | 218 |
| G1 | -1509 | -3342 | 3666.885010 | +0.220955 | 2248.275879 | 230 |
| G2 | -1510 | -3342 | 3667.296631 | +0.206718 | 2248.023682 | 213 |
| G3 | -1510 | -3342 | 3667.296631 | +0.206718 | 2248.023682 | 217 |
| G4 | -1510 | -3342 | 3667.296631 | +0.206718 | 2248.023682 | 217 |
| G5 | -1509 | -3342 | 3666.885010 | +0.220955 | 2248.275879 | 213 |
| H1 | -1509 | -3342 | 3666.885010 | +0.534843 | 2286.818115 | 217 |
| H2 | -1509 | -3341 | 3665.973633 | +0.528412 | 2287.386719 | 217 |
| H3 | -1509 | -3342 | 3666.885010 | +0.534843 | 2286.818115 | 213 |
| H4 | -1509 | -3342 | 3666.885010 | +0.534843 | 2286.818115 | 213 |
| H5 | -1509 | -3341 | 3665.973633 | +0.528412 | 2287.386719 | 213 |

## Resumen y conclusión

| Serie | Impedancia media (Ω) | Desviación estándar muestral (Ω) | Diferencia frente a nominal externo + 100 Ω* |
| --- | ---: | ---: | ---: |
| A | 306.459 | 0.059 | −4.23 % |
| B | 321.285 | 0.049 | +0.40 % |
| C | 435.105 | 0.015 | +35.97 % |
| D | 442.578 | 0.026 | +38.31 % |
| E | 2112.163 | 1.447 | −8.17 % |
| F | 2214.809 | 3.423 | −3.70 % |
| G | 2248.125 | 0.138 | −2.26 % |
| H | 2287.046 | 0.311 | −0.56 % |

*Comparación orientativa con **320 Ω** para A–D y **2300 Ω** para E–H, suponiendo que R2 interviene como una suma resistiva ideal en ambas rutas; no es una corrección aplicada a las medidas.*

Las cinco lecturas de cada serie son repetibles: la dispersión relativa máxima es **0.155 %** (F). Considerar R2 explica parte de la diferencia frente a la resistencia externa nominal: B queda a **+0.40 %** de 320 Ω y H a **−0.56 %** de 2300 Ω bajo el modelo simple. Sin embargo, las diferencias entre rutas y calibraciones son mucho mayores que la dispersión dentro de una serie: con la misma carga externa de 220 Ω nominales, las medias van de **306.459 Ω (A)** a **442.578 Ω (D)**; con 2,2 kΩ nominales, de **2112.163 Ω (E)** a **2287.046 Ω (H)**. Cambiar solo la calibración de E a F desplaza la media **102.646 Ω**. R2 no explica por sí sola estos cambios entre series. Los datos demuestran repetibilidad de la adquisición, pero todavía no validan la exactitud absoluta de la resistencia externa.

## Estado de la matriz

Las **40 adquisiciones** previstas de la matriz A–H están registradas: cinco por cada combinación de resistencia nominal, ruta EC y calibración.

La impedancia indicada es la del circuito aparente (`1 / (GF × magnitud DFT)`); **no se ha restado R2** ni aplicado constante de celda, cálculo de conductividad o compensación térmica. La fase corregida solo compensa el instrumento. Para estimar la resistencia externa a partir de la impedancia aparente habrá que medir el patrón real y verificar cómo interviene R2 en la transferencia eléctrica de ambas rutas antes de aplicar una sustracción de 100 Ω o ajustar la conversión.
