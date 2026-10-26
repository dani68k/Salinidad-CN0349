# Validación — Hito 1D.2

## Qué se compiló y qué falta

Firmware 0.5.2 / Hito 1D.2. La compilación ESP32-S3 valida integración del
firmware, pero no su interacción física con AD5934. Las pruebas host de
`test_ad5934_math` incluyen conteo de puntos, límites de barrido, acumulación DDS,
frecuencia efectiva y fase; su ejecución requiere gcc/g++. La compilación 0.5.1
terminó `SUCCESS` en 9,88 s; RAM 25 264 bytes y Flash 376 926 bytes. La
compilación 0.5.2 terminó `SUCCESS` en 7,89 s; RAM 25 264 bytes y Flash 376 918
bytes. La compilación no valida el hardware.

Los intentos físicos reportados con firmware 0.5.0 y 0.5.1 fallaron al emitir el
primer Increment Frequency, con validación en `0x80`. En 0.5.2 la transición ya
no depende del nibble de operación retenido: el driver exige D1 válido y Control
Low `0x08`, y la aplicación llega a ese paso solo después de leer REAL e IMAG.
La siguiente ejecución física se registró en el apartado de resultados.

## Validación física — barrido predeterminado en 0.5.2

El usuario ejecutó `ad5934 measure sweep config`, `ad5934 measure sweep start`
y `ad5934 measure sweep result` con `CAL_LOW_1K` (0x12), MCLK externo 1 MHz,
start 1000 Hz, step 250 Hz, 8 increments y 9 puntos. El firmware informó
“Barrido completado: 9/9 puntos; AD5934 en standby”. La tabla devolvió nueve
puntos válidos, a frecuencias DDS efectivas de 999,995; 1249,991; 1499,988;
1749,985; 1999,982; 2249,978; 2499,975; 2749,972 y 2999,969 Hz. REAL, IMAG,
magnitud y fase se registraron como DFT cruda.

Esto valida físicamente el flujo digital de barrido, lectura por punto,
incremento, detección de finalización y retorno reportado a standby para la
configuración predeterminada. No verifica resistencia/impedancia real, exactitud
analógica, calibración, repetibilidad ni las demás configuraciones; magnitud y
fase no están convertidas a ohmios.

El AD5934 define **increments**, por lo que total points = increments + 1. La
configuración predeterminada es start 1000 Hz, step 250 Hz, 8 increments: nueve
puntos nominales hasta 3000 Hz. El firmware limita a 64 puntos y exige que el
código DDS acumulado permanezca en 24 bits y dentro de MCLK/16. La frecuencia por
punto reportada se deriva de DDS, por lo que puede diferir ligeramente de la
nominal por cuantización.

## Prueba A — barrido predeterminado

```text
info
ad5934 measure set preset CAL_LOW_1K
ad5934 measure sweep config
ad5934 measure sweep start
ad5934 measure sweep status
```

Consultar `sweep status` hasta COMPLETE o ERROR y luego:

```text
ad5934 measure sweep result
ad5934 status
ad5934 read16 0x80
ad5934 read 0x81
adg715 read
```

Aceptar como completo solo si hay 9 puntos válidos, D2 confirmó final de sweep,
y AD5934 terminó en standby/Control Low `0x08`. Inspeccionar índices 0..8 y
frecuencias próximas a 1000, 1250, …, 3000 Hz. No fijar valores esperados para
REAL/IMAG/magnitud/fase. Los puntos internos de frecuencia usan DDS acumulado.

## Pruebas B–D — repetición, preset y validación

Repetir el barrido predeterminado y guardar todas las filas para comparar
magnitud/punto con la primera ejecución. Luego:

```text
ad5934 measure set preset CAL_HIGH_1K
ad5934 measure sweep config
ad5934 measure sweep start
ad5934 measure sweep status
ad5934 measure sweep result
adg715 read
```

El resultado debe conservar CAL_HIGH_1K/`0x11`, aunque se cambie la configuración
posteriormente. El preset del barrido es siempre el preset compartido de
`ad5934 measure set preset`.

Probar antes de iniciar: `step 0` con increments >0, increments `64`, start o
end fuera del rango DDS/MCLK, y end code acumulado que exceda 24 bits. Cada valor
inválido debe rechazarse sin cambiar la configuración mostrada ni escribir
registros. `increments 0` es un barrido válido de un solo punto; step cero es
admisible solo en ese caso.

Mientras corre un barrido, probar `ad5934 measure sweep set ...`,
`ad5934 measure set preset ...` y `ad5934 measure start`; se deben rechazar por
ocupado. No debe arrancar otra medida en paralelo.

## Prueba E — cancelación y parciales

Iniciar el barrido predeterminado y enviar `ad5934 measure sweep stop` cuando ya
haya al menos un punto almacenado (observar status). Confirmar CANCELLED,
conservación de puntos válidos como resultado parcial, standby seguro y terminal
operativo. `sweep result` no debe etiquetar el conjunto como COMPLETE.

## Prueba F — regresión de adquisición única

```text
ad5934 measure set freq 2000
ad5934 measure start
ad5934 measure status
ad5934 measure result
```

Confirmar que el punto único sigue funcionando y que consultar el último sweep
después de una medida única no lo borra. Solo un nuevo `sweep start` reemplaza el
conjunto guardado.

## Errores y D2

Cada punto tiene timeout D1 de 500 ms desde Start Sweep o Increment Frequency.
Si ocurre error después de algunos puntos, se conservan las filas válidas y el
conjunto queda ERROR parcial. Tras almacenar el último punto se lee Status D2 una
vez. Rev. E define D2 como activo al terminar el procesamiento de la última
frecuencia y lo reinicia Start Sweep; si D2 aún no está activo, se informa error,
se recupera standby y **no** se envía un incremento extra ni se espera sin límite.

Los resultados son DFT cruda. La fase `atan2(IMAG, REAL)` no es fase de
impedancia corregida; no se calculan impedancia, conductancia, conductividad ni
coeficientes de calibración. El readback ADG715 solo confirma el registro digital,
no continuidad analógica. No se reporta ninguna ejecución física de 1D.2 hasta
que el usuario aporte sus salidas del terminal.
