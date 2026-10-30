# Drivers

Los drivers reciben `TwoWire&` y no inicializan el bus ni dependen del
terminal, BLE o la lógica del instrumento.

- [AD5934](AD5934/README.md): registros, control de adquisición y datos DFT.
- [ADG715](ADG715/README.md): lectura y escritura verificada de interruptores.

La secuencia de adquisición y la política de rutas residen respectivamente en
`ImpedanceApp` y `MuxApp`.
