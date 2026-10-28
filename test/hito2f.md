# Hito 2F — validación (2026-10-27)

## Ejecutado sin hardware

| Verificación | Resultado |
|---|---|
| `test\host_cn0349\run.cmd` | 7 grupos PASS |
| `test\host_cn0349\run_math.cmd` | Pt100 3/3 y AD5934 11/11 PASS |
| PlatformIO ESP32-S3 N32R8V | SUCCESS; RAM 23 424 B, Flash 376 762 B |

La suite host enlaza producción y drivers con Arduino/Wire simulados. Comprueba:

- Inicialización, cuatro presets con cinco muestras, GF/IDs/configuración y
  ausencia de READY prematuro; hardware ausente y bus no inicializado.
- Calibración parcial por DFT cero/timeout; slot fallido invalidado sin reutilizar
  GF anterior; EC operativo aunque falle Pt100; recalibración y recuperación.
- Manual durante continuo: cancelación segura, calibración compartida y ausencia
  de reanudación. Rechazo de segundo inicio durante calibración.
- PT100, EC y ambos: selección en Idle, once, continuo y stop; orden 0x41→0x82,
  HIGH 0x81, metadatos y resultados históricos tras setup/recalibración.
- Perfil de frecuencia/settling incompatible, argumentos inválidos, K NaN y
  modelo serie no habilitado; rechazo Busy también entre ciclos.
- Inicio inmediato, límite 999/1000 ms, desbordamiento millis, intervalo de 1 ms
  excedido sin backlog, contador de excedidos y resultados sobrescritos.
- 1000 ciclos secuenciales válidos consumidos sin sobrescrituras ni duplicados.
- Stop durante configuración y ambas adquisiciones; snapshot cancelado inválido,
  standby y posterior medida válida; stop entre ciclos sin más transacciones.
- Fallos I²C, readback ADG715, standby, timeout por sensor, DFT cero y Pt100
  fuera de rango: no se publica ciclo OK y se conserva el otro sensor válido.
- Consultas de terminal sin I²C, normalización, duplicados/desconocidos, comandos
  retirados, Busy, salida compacta, error por sensor, CRLF y desbordamiento de línea.
- Fórmula inversa, fase normalizada, GF/magnitud no finitos o no positivos,
  underflow/overflow, tolerancia numérica DDS, IEC 60751 y RAW/serie como cálculo.

Los binarios host no prueban el circuito, reloj físico, señal de electrodos,
redondeo decimal Arduino ni latencia real bajo actividad serie. Un run de 1000
ciclos simulados no equivale a funcionamiento físico prolongado.

## Pendiente en placa antes de Hito 3

Registrar fecha, placa, revisión, conexión, comando, captura serie y resultado
observado. No marcar superado por compilar o por pasar el doble de Wire.

1. Arranque en frío y reinicio durante calibración/medida: cabecera, CAL 4/4 y
   READY; comprobar MCLK externo y configuración/rutas reales. Repetir sin monitor.
2. Calibrar las cuatro resistencias, revisar GF/fase/variación con snapshots de
   depuración y comprobar repetibilidad. Verificar degradación parcial real.
3. Pt100 real y referencias de resistencia: comparar temperatura IEC y Z sin
   introducir ajustes empíricos. No asumir compensación de tres hilos.
4. EC real en LOW/HIGH y RAW: comprobar señal, impedancia y conductividad
   estimada con K nominal. Mantener serie deshabilitada sin verificar topología.
5. Once y continuo para las tres selecciones; confirmar con instrumentación que
   solo se activa una ruta y que standby precede cada cambio.
6. Stop durante configuración, Pt100, EC y espera entre ciclos: medir retorno a
   standby y receptividad de terminal; cal run durante continuo no debe reanudar.
7. Retirar dispositivos o forzar fallos I²C/DFT de forma controlada: errores,
   ausencia de valores antiguos, recuperación y bloqueo de medidas inseguras.
8. Intervalo menor que el ciclo y funcionamiento prolongado: cadencia entre
   inicios reales, contadores, consumo, terminal receptivo y memoria estable.
9. Verificar formatos decimales de salida sobre Arduino y estabilidad de ambos
   sensores después de reinicios y calibraciones repetidas.

La validación física de calibración, Pt100 y EC sigue pendiente. La calibración
electrónica no sustituye calibración metrológica con patrones, fuera de este hito.
