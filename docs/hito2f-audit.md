# Hito 2F — auditoría previa (2026-10-27)

Revisados main, Terminal, CN0349Manager, sus tres extensiones, ImpedanceApp,
MuxApp, drivers, conversiones, configuración y suites host/Unity.

- Drivers AD5934/ADG715: transacciones acotadas, verificación de registros y
  standby. Se conservan sin cambios.
- ImpedanceApp: única máquina cooperativa de adquisición, actualmente mezclada
  con parser, volcado de registros y barridos experimentales.
- MuxApp: catálogo de máscaras e interlock de control seguro; también contiene
  un segundo parser y salidas extensas. Se conserva el interlock y readback.
- CN0349Manager: calibración de cinco muestras, GF y fase circular reutilizables.
  Duplica adquisición/conversión/publicación en RTD, EC y temperatura continua.
  Depende de Stream/Terminal y conserva configuraciones de sweep/single que no
  necesita la operación final. La calibración aborta al primer fallo y puede
  conservar slots antiguos: se debe invalidar cada intento y continuar por slot.
- Terminal: buffer fijo, presupuesto de lectura y CRLF reutilizables; registro
  de módulos, escáner y comandos históricos se retiran de la interfaz final.
- Tests: suite MSVC compila producción con Arduino/Wire simulados; buena
  cobertura matemática y de fallos, pero las pruebas de comandos históricos y
  estados del gestor deben sustituirse por pruebas del contrato 2F. Unity de
  Pt100 y AD5934 se conserva.

Plan: simplificar el gestor existente como controlador independiente del
terminal; reutilizar la adquisición única; conservar snapshots y fórmulas;
calibrar cuatro slots sin bloquear y con estado por slot; secuenciar ciclos;
reemplazar parsers por Terminal operativo; probar API, fallos, reloj y parser;
compilar PlatformIO y documentar validación física pendiente. Sin BLE ni NVS.
