# ImpedanceApp — adquisición única no bloqueante

Hito 2F, 2026-10-27. Coordina AD5934 y MuxApp. No usa Stream, Serial ni Terminal.
Se mantienen las etapas de la adquisición funcional anterior y los readbacks;
se retiran parser, volcados, barridos y sus buffers/configuraciones.

Interfaz interna: `requestSingle` adquiere una referencia; `requestPt100` y
`requestEc` utilizan exactamente la misma máquina con otra ruta. `update`
progresa una etapa, `cancelOperation` solicita Stopping, `singleResult` entrega
datos solo en Complete. `statusSnapshot` consulta estado sin transacciones.
`configureSingle` y `requestRecovery` se reservan al controlador del instrumento.

Estados: Idle → Configuring → PreparingStandby → Configuring → Initializing
→ Settling → Acquiring → ReadingReal → ReadingImaginary → Stopping → Complete.
Errores y cancelación pasan por Stopping, y terminan Error o Cancelled. Se
comprueba standby incluso al cancelar en power-down. El driver permite ambos
estados inactivos, pero el instrumento exige standby al finalizar.

Se escribe/verifica Range 1/PGA ×1, DDS, cero incrementos y 10 ciclos.
Settling usa millis; se sondea D1 cada 5 ms y vence a 500 ms. Configuración y
control vencen a 1500 ms por fase. Cada transacción I²C está acotada por Wire;
no hay espera activa ni delay. Una caída de I²C o readback fallido invalida la
operación y la readiness cuando no puede verificarse el hardware. La parada
intenta standby una vez y registra RecoveryFailed si no lo consigue.

Dependencias: drivers locales, MuxApp, ad5934_math e impedance_diagnostics.
No convierte temperatura ni conductividad y no programa ciclos periódicos;
esas responsabilidades pertenecen al gestor y las conversiones puras.
