# MuxApp — rutas seguras CN0349

Hito 2F, 2026-10-27. Catálogo de máscaras y política de conmutación por encima
del driver ADG715. No depende del terminal ni imprime. Se retiran parser,
control individual de switches y presentación de diagnósticos.

Interfaz: `resolveCalibrationPreset`, `applyCalibrationPreset`, `pt100Info`,
`applyPt100`, `ecInfo`, `applyEc`, `readMask` y consultas del último estado.
Referencias: 0x09/100Ω, 0x11/1kΩ (RFB 100Ω); 0x12/1kΩ, 0x22/10kΩ (RFB 1kΩ).
Sensores: Pt100 0x41, EC HIGH 0x81, EC LOW 0x82; ALL_OFF 0x00 también es seguro.

Solo escribe máscaras autorizadas y comprueba que AD5934 esté inactivo con
Control Low 0x08. ADG715::writeState conserva la verificación de readback.
Una lectura inicial fallida, interlock inseguro o discrepancia devuelve false.
El último registro observado no equivale a validar eléctricamente la ruta.
El controlador impide adquisiciones concurrentes y secuencia standby antes
de cambiar de sensor. Dependencias: AD5934 y ADG715, sin máquinas DFT propias.
