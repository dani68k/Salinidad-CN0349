# MuxApp

Aplica la política de rutas CN0349 sobre ADG715 y comprueba el estado inactivo
del AD5934 antes de conmutar. No realiza conversiones ni imprime.

| Función | Propósito |
|---|---|
| `resolveCalibrationPreset(id, info)` | Obtiene máscara, RFB y RCAL de una referencia. |
| `pt100Info(info)`, `ecInfo(mask, info)` | Obtienen información de la ruta Pt100 o EC autorizada. |
| `applyCalibrationPreset(id)`, `applyPt100()`, `applyEc(mask)` | Comprueban interlock y aplican la máscara. |
| `readMask(mask)`, `knownPresetMask(mask)` | Leen el registro o comprueban si una máscara es conocida. |
| `hasObservedMask()`, `observedMask()`, `lastDriverError()` | Consultan último estado observado o error. |

Las funciones de aplicación y lectura devuelven `bool`. Referencias:
0x09/100 Ω y 0x11/1 kΩ con RFB 100 Ω; 0x12/1 kΩ y 0x22/10 kΩ con RFB 1 kΩ.
Pt100 usa 0x41, EC HIGH 0x81 y EC LOW 0x82. El driver verifica el readback;
eso confirma el registro digital, no la continuidad analógica.

La API y los tipos públicos están documentados en `MuxApp.h` para la referencia
Doxygen generada con `doxygen Doxyfile` desde la raíz del proyecto.
