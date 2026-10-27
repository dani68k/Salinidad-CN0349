# Hito 2D — validación de temperatura experimental, 2026-10-27

Estado: firmware compilado y pruebas software superadas. **Validación física
de temperatura pendiente**; no se ha cargado firmware ni ejecutado adquisición
en placa durante este desarrollo. Los ensayos anteriores siguen siendo evidencia
del hito correspondiente, no validación del conversor nuevo.

## Preparación y comandos

1. Con el circuito apagado, conectar un rojo de la Pt100 a PT100-1 y el azul a
   PT100-2; aislar el segundo rojo. Se usa un sensor de tres hilos como sensor
   de dos terminales. Comprobar conexiones antes de alimentar.
2. Registrar identificación del sensor, multímetro (modelo, rango, resolución,
   exactitud declarada), temperatura/medio de ensayo y estado térmico estable.
   Desconectar el sensor del CN0349 para medir resistencia con multímetro;
   no conectar el ohmímetro al circuito alimentado. Medir el mismo par de hilos.
   Registrar lectura de puntas cortocircuitadas como observación, sin restarla
   automáticamente del firmware ni usar los 0,6 ohm como offset.
3. Alimentar la placa con este firmware y ejecutar `cn0349 diag`. Si no hay
   calibración RAM tras reiniciar, realizar explícitamente la calibración 2B
   con `cn0349 cal run CAL_HIGH_100R` o `CAL_HIGH_1K`, esperar finalización y
   guardar `cn0349 cal result`. No se recalibra desde `temperature start`.
4. Consultar y seleccionar manualmente:

```text
cn0349 impedance cal list
cn0349 impedance cal set CAL_HIGH_100R
cn0349 temperature help
cn0349 temperature start
cn0349 temperature status
cn0349 temperature result
```

Repetir `status` hasta salir de RUNNING antes de recoger `result`. `stop`
solicita cancelación y espera standby mediante el motor asíncrono existente.
ID=0 significa que no se llegó a aplicar una calibración. El error de la última
solicitud se muestra separado del resultado almacenado; BUSY no cancela ni
reemplaza un intento en curso. Las consultas no hacen I2C.

## Registro y comparación

Guardar cinco adquisiciones por calibración HIGH, sin cambiar cableado ni
condiciones térmicas. Para cada una registrar salida completa, estado/error de
adquisición, error hardware, validez/error matemático, preset/ID/GF aplicado,
REAL/IMAG/magnitud, Z aparente, temperatura, frecuencia efectiva, fase corregida
instrumentalmente y duración. No usar temperatura si la adquisición no es válida
o la conversión no devuelve NONE. Z válida puede quedar fuera del rango Pt100:
en ese caso se conserva adquisición y se informa OUT_OF_RANGE sin temperatura.

Seleccionar después el otro HIGH y consultar el resultado antes de iniciar otra
medida: debe conservar ID, GF y temperatura anteriores. Volver a adquirir y
comparar ambos coeficientes sin ajustarlos. Registrar variación de resistencia
y temperatura, y lecturas del multímetro antes/después para detectar deriva térmica.
La temperatura equivalente del multímetro puede calcularse mediante la misma
ecuación IEC; esa comparación comprueba consistencia resistencia→temperatura,
no constituye una referencia independiente de temperatura. Para validar esta
última se necesita un termómetro de referencia y un entorno térmico controlado.

## Límites

`EXPERIMENTAL / NOT ABSOLUTELY CALIBRATED`. La resistencia de ambos cables y
contactos se suma al elemento y produce error positivo de temperatura; el tercer
hilo aislado no permite compensación. No se aplica ninguna corrección. La fase
instrumental no se usa para sustituir Z por su parte real. La cadena analógica,
posible saturación, señal insuficiente, autocalentamiento y exactitud absoluta
siguen pendientes de caracterización. No deducir precisión física de los tests.

## Verificación software ejecutada

- `pio run -e esp32-s3-devkitc-1-n32r8v`: SUCCESS, RAM 28280 bytes, Flash 410642 bytes.
- `test\host_cn0349\run_math.cmd`: 3 pruebas Pt100 y 11 AD5934 PASS con MSVC/Unity.
- `test\host_cn0349\run.cmd`: cuatro grupos PASS, con adquisición/terminal reales
  compilados y Arduino/Wire simulados. Incluye errores, cancelación, rango,
  snapshot inmutable tras selección/recalibración/otra medida, y regresiones.
- `pio test -e native`: ERRORED por ausencia de gcc/g++; no se presenta como PASS.
- Warning MSVC C4018 preexistente en AD5934.cpp, sin modificar el driver.

Resultados físicos nuevos: pendientes. Adjuntar aquí transcripciones, condiciones
y referencias instrumentales cuando se realice el ensayo.
