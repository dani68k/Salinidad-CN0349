# Terminal

`Terminal(Stream&, CN0349Manager&)` interpreta comandos serie y utiliza la
API del controlador. `begin()` imprime la cabecera y `update()` procesa
entrada y publica resultados. No accede directamente a Wire ni a los drivers.

```text
help | status
setup show | setup interval <ms>
setup ec range high|low
setup ec model raw|series|ad|analog_devices
setup ec k <K>
setup compensation on|off
setup alpha <0..5>
cal status | cal show | cal run
measure show | measure set PT100|EC|PT100 EC
measure once | measure start | measure stop
```

`measure set` acepta ambos sensores en cualquier orden. Los comandos se
normalizan a minúsculas; el buffer admite 95 caracteres más terminador. Setup
y selección son validados por el controlador, incluida la dependencia de ambos
sensores cuando EC25 está activa. Los errores se presentan explícitamente;
un fallo de persistencia indica que Setup quedó aplicado en RAM.

`cal show` muestra GF legacy y coeficientes AD; los ciclos muestran Pt100,
EC y, con compensación activa, EC25 o `unavailable`. Los ciclos cancelados
no se imprimen. El terminal consume el slot de resultados; BLE consulta el
último snapshot sin consumirlo.

La interfaz pública de `Terminal.h` forma parte de la referencia HTML generada
con `doxygen Doxyfile` desde la raíz del proyecto.
