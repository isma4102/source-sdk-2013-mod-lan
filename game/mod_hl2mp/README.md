# Supervivencia Paysandu

Cooperativo LAN sobre Half-Life 2: Deathmatch (Source SDK Base 2013 Multiplayer). Hambre, sed y stamina, mochila, zombies, cajas, día y noche. El mapa de Paysandú se hace aparte en Hammer; hasta entonces el mapa de prueba es `dm_lockdown`.

## Instalar

1. Instala **Source SDK Base 2013 Multiplayer** en Steam (AppID 243750).
2. Copia la carpeta `game/mod_hl2mp` a `steamapps/sourcemods/mod_hl2mp`.
3. Reinicia Steam. El mod aparece como **Supervivencia Paysandu**.
4. Los dos PCs necesitan la misma carpeta del mod y el SDK Base.

Si ya estaba instalado, no basta con dejar el juego abierto. Cierra el mod, copia otra vez `game/mod_hl2mp` encima de `steamapps/sourcemods/mod_hl2mp` (sustituye los archivos) y reinicia Steam. El menú lee `gameinfo.txt`, `resource/ClientScheme.res` y los VTF al arrancar. Esta fase cambia el sprint en `client.dll` y `server.dll`: hay que recompilar y copiar `bin/`. El icono de la biblioteca sigue siendo `resource/icon.tga` y `logo.png`: Steam solo lo refresca al reiniciar.

Al abrir el mod, el fondo es la calle de Paysandú (`materials/console/background01`, y `background01_widescreen` en 16:9). El cuadro entra entero, con barras si la proporción no coincide. El título del menú es **SUPERVIVENCIA** / **PAYSANDU**, en ASCII: GameUI pinta `title` y `title2` con `ClientTitleFont`, y la fuente `HL2MP` del SDK solo trae el logo y los iconos de muerte.

Los binarios de Linux van en `bin/linux64/client.so` y `bin/linux64/server.so`. En Windows hace falta compilar `client.dll` y `server.dll` con el SDK.

## Hostear en LAN (2 jugadores)

1. En la consola, antes de crear la partida: `sv_lan 1`. En el menú de crear servidor marca LAN.
2. Mapa: `dm_lockdown`.
3. Al arrancar el listen server, el motor ejecuta `cfg/listenserver.cfg`, que a su vez ejecuta `cfg/survival_coop.cfg`. Eso deja el coop, las necesidades, el arsenal realista y el reloj de 12 minutos por día.
4. El segundo jugador abre el mod, pestaña **LAN** en buscar servidores, o en consola `connect IP:27015` con la IP del host.

`listenserver.cfg` no cambia de mapa. Si la config no se aplicó, en consola: `exec survival_coop`.

## Controles de supervivencia

```
bind g survival_eat
bind h survival_drink
```

E (`+use`) guarda comida y agua en la mochila. Caminar encima no las consume. `survival_eat` y `survival_drink` gastan un slot. Al reaparecer, hambre, sed y stamina vuelven a 100 y la mochila se vacía.

La barra de cansancio es stamina que queda: 100 es fresco y 90 sigue siendo casi lleno. El sprint no gasta la energía del traje HEV (`sv_survival_sprint_uses_suit 0`). Se gastan 5 puntos por segundo (`sv_survival_stamina_drain_sprint`): unos 11 s a velocidad plena. Por debajo de 45 (`sv_survival_stamina_slow_at`) la velocidad baja en línea hasta 0.75 en 0. El sprint se corta en 1 (`sv_survival_stamina_sprint_min`). Quieto se recupera a 10/s. Con hambre o sed en 0 no se regenera y el jugador recibe daño. `sv_survival_sprint_uses_suit 1` devuelve el corte por el traje.

## Arsenal

Por defecto (`sv_survival_realistic_weapons 1`) el spawn y el `give` entregan pistola, revólver 357, SMG, escopeta, palanca y granada de fragmentación. Quedan fuera gravity gun, rifle de pulso, bola de energía, RPG, ballesta, stunstick y SLAM. `sv_survival_realistic_weapons 0` devuelve ese set.

## Día, noche y cajas

El reloj del HUD empieza a las 08:00. `sv_survival_day_length 720` es un día completo. De noche la pantalla se oscurece.

En Hammer (más adelante): `survival_crate` suelta comida y agua, `survival_bed` duerme. Para probar ya: `sv_cheats 1` y `ent_create survival_bed` o `survival_sleep`. Dormir llena la stamina y suma un poco de hambre y sed. Entre las 20:00 y las 07:00 el reloj salta a las 07:00.

Zombies de prueba (cheat): `survival_spawn_zombie` con `zombie`, `fast`, `poison`, `zombine`, `torso` o `headcrab`.

## Combate de zombis

HL2MP no inicializaba la tabla de relaciones de HL2, así que un zombi veía al jugador como `D_ER` y solo se movía en el sitio. Además `sk_zombie_health` nacía en 0: un disparo de pistola (8 de daño, 16 a la cabeza) lo mataba.

Con `sv_survival_zombies_hate_players 1` (el valor por defecto) el zombi persigue y muerde. `sk_zombie_health 80` aguanta varios tiros de pistola. `sk_zombie_health 1` vuelve a dejarlo a un disparo. La mordida usa `sk_zombie_dmg_one_slash` (10).

`dm_lockdown` no trae grafo de `info_node`. El zombi avanza a pasos hacia el jugador (`sv_survival_nodeless_chase 1`) sin esa red. `nav_generate` arma la malla de NextBot, que estos NPC no usan. No hace falta, y `ent_create info_node` puede cerrar el juego.

Prueba, con el mapa ya cargado:

```
exec survival_coop
sv_cheats 1
survival_spawn_zombie
```

Retrocede: el zombi tiene que caminar y golpear. Un tiro de pistola al cuerpo no lo mata. `sk_zombie_health` en consola muestra 80.

## Mordida, ruido, días y revivir

El HUD muestra hambre, sed, cansancio, infección y el día compartido (`Día N` más la hora). El día no se reinicia cuando muere un jugador: es el mismo para la partida. Cambia de mapa o `mp_restartgame` lo vuelve al día 1, a las 08:00.

Un golpe de `npc_zombie`, torso, fast, poison o zombine puede infectar (`sv_survival_infect_chance`). La barra sube sola y, al llegar a 100, mata. Mientras tanto resta un poco de vida. Al reaparecer la infección vuelve a 0.

Sprint, disparos, abrir una `survival_crate` y un +use fuerte atraen zombis dentro del radio. De noche (20:00 a 06:00) ese radio se multiplica. No hay un pulso por frame: `sv_survival_noise_cooldown` junta los ruidos.

El primer golpe que dejaría la vida en 0 no mata: el jugador queda herido, se arrastra y no dispara. Un compañero mira al herido y mantiene E unos segundos (`sv_survival_revive_time`) para levantarlo con poca vida. La infección no se cura al revivir. Si nadie llega a tiempo, muere y reaparece con las necesidades en 100. El fuego amigo sigue cortado por `sv_survival_coop` y `mp_friendlyfire 0`.

Con `sv_cheats 1`:

```
survival_infect
survival_infect 80
survival_clear_infection
survival_down
survival_noise
survival_dump
cl_survival_dump
```

El nombre del mod, en Steam y en la ventana, es **Supervivencia Paysandu**. El `hostname` del listen server usa el mismo ASCII porque la consola del motor no siempre acepta la ú. Los textos del HUD en español siguen en `resource/mod_hl2mp_spanish.txt` (UTF-16); el menú principal no los lee.

## Mapa de prueba

`mapcycle.txt` empieza por `dm_lockdown` y sigue con mapas de HL2MP que ya vienen en el SDK. El mapa de Paysandú sustituirá a `dm_lockdown` cuando exista.
