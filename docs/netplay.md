# Multijugador online / LAN (hasta 4 jugadores)

Funciona por UDP, sirve para red local y para Radmin VPN (o cualquier VPN/IP directa).

## Cómo se juega

1. **Todos** usan exactamente el mismo build (el mismo .zip de GitHub Actions) y la misma configuración
   del juego (mejoras/enhancements, región US). Si algo de la simulación difiere, la partida se desincroniza.
2. **Host (tú = Jugador 1)**: ESC → menú **Online** → pon tu nombre, puerto (7777) y pulsa **Host game**.
   Abre el puerto UDP en el firewall de Windows (la primera vez Windows pregunta: permitir redes privadas).
3. **Clientes (Jugador 2, 3, 4)**: ESC → **Online** → escriben la **IP Radmin del host** (26.x.x.x),
   el mismo puerto, eligen personaje/traje y pulsan **Join game**. Verán la lista de jugadores.
4. El host entra al **VS → selección de personaje**, elige su personaje, reglas y escenario y pulsa
   **Start** como siempre. Los clientes conectados son metidos automáticamente en la partida con el
   personaje que eligieron (aunque en tu pantalla de selección esos puestos aparezcan vacíos).
5. Cada jugador usa **su propio control en el puerto 1** de su PC.
6. Al terminar la partida cada uno vuelve a su pantalla de resultados; el host puede iniciar otra
   desde la selección de personaje sin que nadie tenga que reconectarse.
7. Si alguien se desconecta durante la partida (más de ~8 s sin paquetes) la partida se cancela para
   todos y se vuelve al menú.

Puestos de jugador que el host deje como "Human" pero sin cliente conectado se quedan quietos; los
puestos CPU del host siguen siendo CPU.

## Cómo funciona por dentro

**Lockstep determinista + input delay** (el mismo modelo que el netplay de juegos de pelea simples,
sin rollback):

* Todas las máquinas simulan el juego completo. Solo se intercambian los **controles** (botones +
  stick) de cada jugador, nunca el estado del juego. El juego ya es determinista (la gente lo usa
  para replays), así que mismos inputs + misma semilla aleatoria = misma partida.
* **Input delay (por defecto 5 frames)**: lo que pulsas en el frame T se aplica en el frame T+5 en
  *todas* las PCs, incluida la tuya. Ese margen es el tiempo que tienen los paquetes para llegar.
  Si para el frame T falta el input de alguien, el juego espera (se congela ese frame) hasta que llegue.
  Puedes subir el delay en el host si hay stalls.
* **Topología en estrella**: los clientes solo hablan con el host, que reenvía los inputs a los demás.
  Por eso solo necesitas la IP del host.
* **UDP con redundancia**: cada paquete lleva los inputs que el otro lado aún no confirmó (ack), así
  que perder paquetes solo añade algo de espera, nunca desincroniza (probado con 40% de pérdida).
* **Inicio de partida**: cuando el host pulsa Start en la selección de personaje, empaqueta la
  configuración (escenario, reglas, personajes de todos, semilla aleatoria) y la envía a los clientes.
  Todos (host incluido) construyen la partida desde exactamente los mismos datos, cargan, y avisan
  "READY"; cuando todos están listos el host manda START y arranca el frame 0 al mismo tiempo.

## Archivos

| Archivo | Qué hace |
|---|---|
| `port/netplay/netplay_session.{hpp,cpp}` | Transporte UDP (Winsock/BSD): lobby, config, barrera de inicio, inputs, abort |
| `port/netplay/netplay.{h,cpp}` | API en C sobre esa clase |
| `port/gui/PortMenu.cpp` | Panel ESC → Online (Host/Join) |
| `port/gameloop.cpp` | Llama `syNetLobbyFrame()` una vez por frame |
| `patches/decomp-netplay.patch` | Cambios al decomp: `sys/netlobby.c` (pegamento), `sys/netinput.c` (lectura lockstep), `scvsbattle.c`, `scmanager.c`, `netpeer.c` |
| `tools/netplay_test.cpp` | Prueba local de 4 peers con pérdida de paquetes (se ejecuta en el workflow) |
| `.github/workflows/netplay-windows.yml` | Compila el .exe para Windows y sube el zip |

## Limitaciones conocidas

* No hay detección automática de desincronización ni rollback: con ping muy alto (>150 ms) habrá
  esperas. Sube el input delay.
* Sin NAT traversal: necesita IP alcanzable (LAN, Radmin, ZeroTier, Hamachi o puerto abierto).
* El mando local siempre es el del puerto 1. Cada jugador debe tener los mismos ajustes que afecten
  la simulación (por ejemplo, spawn neutral o cambios de reglas del menú de mejoras).
* Los personajes de los clientes se eligen desde el menú Online, no desde la pantalla de selección.
