# ATLANTES — Instrucciones para Codex

## Proyecto y fuentes de verdad

- ATLANTES: instrumento de análisis de agua con firmware ESP32-S3, PlatformIO y Arduino; integración con WebApp BLE.
- El código actual es la fuente de verdad para nombres, interfaces y estructura. Consultar la documentación técnica pertinente antes de cambiar contratos.
- `README.md` es el diario cronológico del desarrollo; la documentación técnica explica arquitectura, protocolos y módulos.
- No asumir que una función descrita en hitos anteriores está implementada: comprobar únicamente los archivos relevantes.

## Estilo y arquitectura

- Preferir C procedural en C++: funciones, `struct`, `enum` y `namespace`. Usar clases/objetos cuando Arduino o una biblioteca lo requieran o aporten una ventaja clara.
- Separar drivers de hardware (I2C/SPI/registros), lógica funcional de sensores (adquisición/cálculos), aplicación (estados/coordinación) e interfaces (terminal/BLE/WebApp).
- Terminal y BLE deben compartir la API de aplicación; no duplicar lógica de negocio ni adquisición.
- Reutilizar módulos existentes. Evitar nuevas capas, dependencias, asignaciones dinámicas y refactorizaciones no solicitadas.
- Preferir máquinas de estados no bloqueantes y temporización con `millis()`. Evitar `delay()`, esperas activas y nuevas tareas FreeRTOS salvo necesidad justificada.
- Gestionar errores y timeouts explícitamente; dejar el hardware en estado seguro al detenerse o fallar.
- No cambiar fórmulas metrológicas, constantes, calibraciones, rutas analógicas o protocolos fuera del alcance autorizado.

## Alcance y consumo de cuota

- Prioridad: resolver correctamente **la tarea concreta** con el mínimo contexto y las mínimas operaciones necesarias.
- Leer primero los archivos indicados en el prompt y sus dependencias directas. No recorrer todo el repositorio ni realizar auditorías generales por defecto.
- Hacer cambios pequeños y localizados; no reformatear archivos completos ni corregir problemas ajenos al encargo.
- No investigar múltiples alternativas si la solución ya está especificada.
- Si es imprescindible modificar módulos fuera del alcance, cambiar una interfaz pública o tomar una decisión técnica importante, **detenerse y preguntar** antes de ampliar el trabajo.
- No establecer límites artificiales de archivos o tokens: priorizar la corrección. `AGENTS.md` no impone un límite real de cuota.
- Evitar mensajes de progreso, informes y planes extensos salvo que el usuario los solicite. Para tareas claras, implementar directamente.

## Compilación, tests y validación

- **No ejecutar compilaciones ni tests automáticamente**, incluidos `pio run`, `pio test` y análisis masivos.
- El usuario revisará el diff, compilará y probará manualmente en VS Code/PlatformIO.
- Se permite escribir o actualizar tests relacionados con el cambio cuando el prompt lo requiera, pero **no ejecutarlos** sin autorización expresa.
- Si ejecutar un comando de validación resulta imprescindible, solicitar permiso antes.
- No afirmar que compila, que pasa tests o que está validado en hardware si no se ha comprobado.
- Ante errores comunicados por el usuario, analizar solo el error relevante y corregirlo sin ampliar el alcance.

## Documentación

- Mantener un `README.md` en cada módulo o carpeta de desarrollo que requiera documentación técnica.
- Al modificar un módulo, actualizar únicamente su `README.md` con las nuevas funciones, interfaces o cambios de comportamiento implementados.
- Documentar de forma breve y técnica: propósito de las funciones, parámetros, valores devueltos y limitaciones relevantes.
- Respetar la documentación existente y evitar duplicaciones o reescrituras completas.
- No crear ni mantener diarios cronológicos de desarrollo.
- No actualizar automáticamente el `README.md` principal ni documentos globales de arquitectura.
- No crear documentación adicional salvo petición explícita.
- No documentar funcionalidades previstas que todavía no estén implementadas.
- Al finalizar, indicar brevemente los archivos modificados y las pruebas manuales pendientes, sin generar informes adicionales.

## Flujo de trabajo

1. Recibir un microhito con objetivo, alcance, restricciones y aceptación.
2. Localizar y leer solo el código y documentación necesarios.
3. Implementar el cambio mínimo coherente y actualizar la documentación afectada.
4. Entregar un resumen breve para revisión y compilación manual.
5. Corregir en el mismo hilo los errores inmediatos de ese microhito; utilizar un hilo nuevo para el siguiente microhito independiente.

## Formato de entrega

Responder de forma concisa con:

- **Cambios:** qué se implementó.
- **Archivos:** lista de archivos modificados.
- **Validación manual:** comandos o pasos concretos que debe ejecutar el usuario; indicar que no se han ejecutado.
- **Pendientes/riesgos:** solo los relevantes.

No crear informes o archivos adicionales fuera de los exigidos por el prompt y las reglas de documentación.

## Compatibilidad con la arquitectura existente

- La arquitectura actual del proyecto es la referencia principal.
- No modificar la arquitectura existente por preferencias de estilo.
- Respetar las clases, objetos e interfaces ya implementadas.
- Reutilizar los módulos existentes antes de crear otros nuevos.
- Preferir programación procedural con namespaces para módulos nuevos, cuando resulte apropiado.
- No convertir clases existentes a namespaces automáticamente.
- No realizar refactorizaciones arquitectónicas sin autorización explícita.
- Si una nueva funcionalidad requiere cambios importantes de arquitectura, explicar la propuesta y esperar aprobación.
- Priorizar compatibilidad, claridad y mantenimiento del código existente.