# Hito 2C.1 — Plan y evidencia de pruebas

Fecha: 2026-10-27. Estado: implementación y simulación listas; ensayo físico pendiente.

## Evidencia automatizada

- PlatformIO ESP32-S3: SUCCESS (ver diario principal para tamaño final).
- Host MSVC: `test\host_cn0349\run.cmd`, tres grupos PASS. Compila las capas
  reales con Stream/reloj/I²C simulados. [Cobertura y límites](host_cn0349/README.md).
- `pio test -e native`: no compiló; gcc/g++ ausentes. No se declara aprobado.
- `git diff --check`: sin errores de espacios.

## Ensayo físico pendiente (no realizado por Codex)

1. Revisar [circuito y conexión](../docs/pt100-route.md). Obtener el esquema completo
   EVAL-CN0349-PMDZ y confirmar J1, PT100-1/PT100-2, numeración/orientación física.
   La guía oficial indica pines 2 y 3; no conectar a 4/5 (conductividad).
2. Medir por separado las resistencias nominales de 220 Ω y 2200 Ω con multímetro.
   Registrar valor real, resolución del instrumento, fecha, cableado y temperatura
   ambiente si se dispone de ella. No introducir esos valores en el firmware.
3. Compilar/subir explícitamente el firmware 0.9.0 al ESP32-S3. Monitor 115200;
   comprobar alimentación, GPIO4/5, MCLK externo 1 MHz e I²C 100 kHz/25 ms.
4. Ejecutar `cn0349 recover`, `cn0349 diag`, `cn0349 impedance help` y `config`.
   Debe estar ready. El perfil actual debe indicar 2000 Hz, Range 1, PGA x1,
   settling 10; si se modificó, restaurarlo mediante la API/configuración heredada.
5. Ejecutar `cn0349 cal run all`; esperar COMPLETE y guardar `cal list`/resultados
   de cada slot. Las calibraciones son RAM y hay que repetirlas tras reinicio.
6. `cn0349 impedance cal list` debe ofrecer HIGH disponibles y LOW incompatibles.
   Probar `cal set inexistente` y `cal set CAL_LOW_1K`: rechazos explícitos sin
   modificar selección/calibraciones. Los comandos incluyen el prefijo
   `cn0349 impedance`.
7. Con placa apagada, conectar **solo una** resistencia entre los terminales
   Pt100 comprobados; encender y repetir calibraciones RAM si se reinició.
8. Ejecutar la secuencia inferior con cada HIGH y varias repeticiones por carga.
   Consultar status durante la adquisición; debe seguir respondiendo. Verificar
   `0x41` y standby al terminar, DFT originales y COMPLETE/VALID. No considerar
   la máscara digital como prueba de conexión analógica.

```text
cn0349 impedance cal set CAL_HIGH_100R
cn0349 impedance start
cn0349 impedance status
cn0349 impedance result
cn0349 impedance cal set CAL_HIGH_1K
cn0349 impedance start
cn0349 impedance result
```

9. Repetir el paso 8 para 220 Ω y 2200 Ω nominales. **Estas dos medidas y la
   comparación entre calibraciones permanecen pendientes de hardware real.**
10. Durante una medida enviar otro start, cal run, measure single y cambio de
    calibración: deben rechazarse por busy. Consultar help/config/result sin
    perturbar la adquisición. Cancelar con `cn0349 impedance stop`, esperar
    CANCELLED y standby, luego reiniciar medida; no debe heredar cancelación.
11. Comprobar último válido conservado tras cancelar y, con un montaje de fallo
    controlado, tras error. Un fallo de standby debe ser FAILED; recuperar y
    diagnosticar antes de continuar. No se presupone standby si falla I²C.
12. Ejecutar `cn0349 diag`, `ad5934 status`, `ad5934 errors`, `adg715 read`.
    Guardar transcript. Comprobar comandos anteriores single, sweep, cal y
    diagnóstico. Confirmar que IDs, muestras y GF de los cuatro slots se conservan
    durante medidas externas (hasta recalibrar/clear/reiniciar).

## Registro a completar

| R nominal | R multímetro Ω | Cal/ID/GF | Hz solicitada/efectiva | REAL/IMAG/M | Fases raw/cal/corregida ° | Z aparente Ω | Estado/error/ms |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 220 Ω | 215 Ω | HIGH_100R / ID 1 / 0,000001220961 | 2000 / 1999,996582 | −1607 / −3542 / 3889,500244 | −114,403725 / −114,405861 / +0,002136 | 210,573898 | COMPLETE / NONE / 226 |
| 220 Ω | 215 Ω | HIGH_1K / ID 2 / 0,000001164391 | 2000 / 1999,996582 | −1606 / −3546 / 3892,730713 | −114,365997 / −114,738792 / +0,372795 | 220,620987 | COMPLETE / NONE / 219 |
| 2200 Ω | 2150 Ω | HIGH_100R / ID 1 / 0,000001220961 | 2000 / 1999,996582 | −171 / −367 / 404,882690 | −114,982697 / −114,405861 / −0,576836 | 2022,875488 | COMPLETE / NONE / 214 |
| 2200 Ω | 2150 Ω | HIGH_1K / ID 2 / 0,000001164391 | 2000 / 1999,996582 | −171 / −367 / 404,882690 | −114,982697 / −114,738792 / −0,243904 | 2121,152832 | COMPLETE / NONE / 214 |

Comparar `Z_aparente - R_multímetro`, diferencias entre GF/calibraciones y
repetibilidad; no imponer una precisión no especificada ni ajustar GF a lo esperado.
Una discrepancia debe conservar datos y motivar revisión de topología/configuración.
No avanzar a 2C.2. No se calcula temperatura.

## Evidencia física añadida — 2026-10-27

Primera muestra externa reportada por el usuario: resistencia nominal 220 Ω,
multímetro 215 Ω, HIGH_100R, Z aparente 210,573898 Ω, diferencia −4,426102 Ω
(−2,05865 % respecto a la referencia indicada). D1 válido, máscara RTD 0x41,
COMPLETE/VALID/NONE y standby reportados. [Transcript íntegro](hito2c1-220r-transcript.txt).
No se ha medido la repetibilidad de la carga ni la incertidumbre del multímetro.
La segunda calibración HIGH_1K está disponible, pero no se utilizó para medida
externa en este transcript. Quedan pendientes la comparación de calibraciones,
la carga 2200 Ω y el resto del plan. Las secciones anteriores conservan el estado
histórico de entrega; esta entrada actualiza únicamente la evidencia aportada.

### Comparación física HIGH_1K añadida — 2026-10-27

El usuario reportó la misma carga (215 Ω en multímetro) con HIGH_1K ID 2:
REAL −1606, IMAG −3546, magnitud 3892,730713, GF 0,000001164391,
fase DFT −114,365997°, fase de calibración −114,738792°, fase corregida
+0,372795°, Z aparente 220,620987 Ω; COMPLETE/VALID/NONE y standby.
Diferencia con referencia: +5,620987 Ω (+2,61441 %). Diferencia respecto a
HIGH_100R anterior: +10,047089 Ω; magnitud DFT +0,0831 %.
La primera comparación entre calibraciones para esta carga queda realizada;
repetibilidad, causa de la discrepancia, carga 2200 Ω y demás ensayos pendientes.
No se modifica el GF ni se aplica promedio/corrección empírica.

### Primera medida de la carga 2,2 kΩ — 2026-10-27

Referencia reportada por el usuario: multímetro 2,15 kΩ (2150 Ω). HIGH_1K ID 2,
GF 0,000001164391; REAL −171, IMAG −367, magnitud 404,882690,
fase DFT −114,982697°, calibración −114,738792°, corregida −0,243904°.
Resultado 2121,152832 Ω: diferencia −28,847168 Ω (−1,34173 %).
COMPLETE/VALID/NONE, RTD 0x41 y standby reportados; duración Manager 214 ms.
La primera muestra de esta carga con HIGH_1K queda realizada. Pendientes
HIGH_100R para esta carga, repeticiones y demás ensayos del plan. No se deduce
ni aplica una corrección empírica a partir de estas muestras.

### Matriz inicial completada — 2026-10-27

Carga nominal 2,2 kΩ, referencia 2150 Ω, con HIGH_100R ID 1/GF 0,000001220961:
REAL −171, IMAG −367, magnitud 404,882690; fase DFT −114,982697°,
calibración −114,405861°, corregida −0,576836°. Z aparente 2022,875488 Ω,
diferencia −127,124512 Ω (−5,91277 %). RTD 0x41, COMPLETE/VALID/NONE,
standby y duración Manager 214 ms reportados.
La DFT coincide con la muestra HIGH_1K anterior: los 98,277344 Ω de diferencia
entre resultados proceden del GF aplicado. Queda completa la primera matriz
2 cargas × 2 calibraciones; una adquisición por combinación no caracteriza
repetibilidad ni precisión absoluta. Siguen pendientes repeticiones y resto del
plan físico. No se modifican coeficientes ni firmware ni se avanza a 2C.2.

### Serie de repetibilidad planificada

La nueva serie de cinco adquisiciones por combinación se realizará y comentará
en [hito2c.1_testing.md](hito2c.1_testing.md). Las cuatro muestras preliminares
anteriores se conservan por separado y no se cuentan en esa serie de veinte.
