# Drivers de dispositivos

Los drivers son independientes de terminal, BLE y lógica de aplicación.
`AD5934/` implementa acceso protegido a registros, control DFT y lectura de
resultados crudos sin conversión a impedancia.
`ADG715/` implementa control genérico del registro de interruptores con
lectura, escritura verificada y errores; la política de rutas CN0349 reside en
`src/mux/`.
