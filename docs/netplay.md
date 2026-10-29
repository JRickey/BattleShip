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

## Modos y número de jugadores

* **Max players (2–4)**: el host lo elige antes de crear la sala (VS Mode). Los demás reciben "sala llena".
* **VS Mode**: 2–4 jugadores se pelean.
* **1P Mode (co-op, experimental)**: tú y **un** amigo juegan el modo historia como aliados (el port solo
  soporta co-op de 2 jugadores). Toda la partida 1P es una sola sesión sincronizada. Si algo se desincroniza
  (por ejemplo por datos de guardado distintos al desbloquear cosas) la partida se cancela para los dos.
  El host entra por 1P Mode → pantalla de selección, elige personaje/dificultad y pulsa Start.

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

## Update: desync, camera, friendly fire, live mode switch
- RNG is re-seeded on both peers at session start and every lockstep tick (hash of match seed + tick), so stray RNG draws (draw code, local effects) can no longer accumulate into a desync (VS, Polygon team, Master Hand).
- Bonus stage (co-op): if the followed player is eliminated, the camera retargets to a surviving player.
- Friendly fire (1P online): host checkbox, default OFF, synced to the client in the run config.
- The host can switch VS <-> 1P in the room window without closing it (1P needs <= 2 players).
- 1P with 3-4 players is NOT implemented: the co-op code supports exactly one extra human (`coop_player2`).

## Update 3: 1P Mode with 3 players, roster strip, host costume
- 1P Mode online now supports up to 3 players (host + 2 friends). New `coop_player3` fields in `SCSceneData`; helpers in `sc1pmanager.c` (`sc1PManagerIsCoop3`, `IsCoopPartner`, `CoopAnyAlive/AllOut`, `CoopEnemyCap`).
- With 3 humans only ONE port is left for enemies: team stages (Yoshi/Polygon) spawn 1 enemy at a time (the rest through respawn), and Giant DK's stage has no CPU ally. 4 humans is impossible (every enemy needs a player port).
- Character select shows a strip with connected players (name, fighter, costume).
- Host can pick the P1 costume in the Online menu (or leave it to the character select screen).
- The 1P intro screen only shows P2's card; P3 appears when the stage starts.

## Update 4: local ports 2-4 switched off while in a room
While hosting or connected to a room, only physical controller port 0 is read (it drives your online slot); ports 2-4 report neutral input (`syControllerReadDeviceData`, `syNetLobbyIsRoomOpen`). They work again as soon as the room is closed or you leave.

## Update 5: players on the native character select panels
In a room, every connected player is shown on the game's own character select panels (fighter model, name emblem, costume) with the player's name above the panel (`mnPlayersVSNetSyncPanels`). The host's real P1 selection is broadcast to the clients (`syNetLobbyPublishHostChoice`), so clients see P1 live. Clients' own pad 0 is neutral in the lobby so they cannot move the (hidden) cursors. Toggle: "Show connected players on the character select panels" in the Online menu (CVar `gNetplay.ShowPanels`); when off, the old name strip is drawn. 1P Mode allows panels for P1-P3.

## Update 6: desync detection + invisible panel fighters
- Every 30 ticks both peers hash the simulation state (scene, stage, time, per-player stocks, position, status, damage). Clients send the hash to the host (`T_SYNC`); on mismatch the host cancels the run/match for everybody with "Desync detected at tick N". Every 60 ticks each peer also logs `SSB64 NetSync` lines (per-player x/y/status/damage + hash): diff host and client logs to find the first diverging tick.
- Character select panels: the game resets a panel when it (re)creates a cursor for a connected controller port, which left remote fighters invisible. The sync now verifies the real slot state every frame and re-applies when needed.

## Update 7: 1P desync root cause (save data / settings differ per PC)
The 1P flow reads each PC's own save: `unlock_mask` (which challenger stages Ness/Captain/Jigglypuff appear), `fighter_mask` (which fighters are drawn as allies/enemies), bonus records (sound-test unlock check), hiscores and `spgame_time_limit`. Two PCs with different saves walk different stage sequences, so the game desyncs "whenever it wants" and never recovers (VS is immune: everything goes through the metadata blob). Now the host's values travel in the run config; a client plays the run with them and gets its own save back (and re-written) when the run ends. Also a fingerprint of the simulation-affecting port settings (competitive ruleset, neutral spawns, CPU level 9, port bonus stages, unlock-all cheat, tap-jump) is part of the periodic state hash, so mismatching settings are reported as a desync at tick 0.

## Music overrides (mods/music)

Replace any BGM with an MP3/OGG/WAV: put `music/<bgmId>.mp3` (or `.ogg`/`.wav`) in `<app-data>/mods/` as loose files, or inside any `.zip`/`.o2r` in `mods/`. Use `<id>_once.mp3` for non-looping jingles. IDs follow `gmsound.h` (0 Dream Land, 1 Zebes, 2 Mushroom Kingdom, 6 Castle, ...). Disable with CVar `gMusicOverride.Enabled=0`. Source: `port/audio/music_override.cpp` (minimp3 + stb_vorbis in `port/audio/third_party`); hooks in `decomp/src/sys/audio.c` (`syAudioPlayBGM`, `syAudioStopBGM`, volume setters) and the mix in `portAudioSubmitFrame`.

### Audio Editor + gameplay cheats

- **Menu > Assets > Audio Editor**: pick the music file for every BGM id (Default / Original / Random custom / any file in `mods/music`, `.zip` or `.o2r`). Stored in CVars `gMusicOverride.Sel.<id>`. Hi-res texture packs can now also be `.o2r`.
- **Menu > Settings > Cheats > Gameplay Cheats (offline only)**: infinite jumps, infinite shield, always 0%, infinite stocks/lives, CPU fighters at 999%. Applied in `ftMainProcUpdateInterrupt` / `ftCommonDeadProcUpdate` and forced off while an online room is open (`syNetLobbyIsRoomOpen`).

## Web asset editor

`web/index.html` is a static page (English default, Español option). GitHub Pages deploys it through `.github/workflows/pages.yml`. When you change the editor, copy the new build over `web/index.html` and push.
