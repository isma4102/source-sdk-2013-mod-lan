# Supervivencia Paysandu

Cooperativo LAN sobre Half-Life 2: Deathmatch (Source SDK Base 2013 Multiplayer). Hambre, sed y stamina, mochila, zombies, cajas, día y noche. El mapa de Paysandú se hace aparte en Hammer; hasta entonces el mapa de prueba es `dm_lockdown`.

## Instalar

1. Instala **Source SDK Base 2013 Multiplayer** en Steam (AppID 243750).
2. Copia la carpeta `game/mod_hl2mp` a `steamapps/sourcemods/mod_hl2mp`.
3. Reinicia Steam. El mod aparece como **Supervivencia Paysandu**.
4. Los dos PCs necesitan la misma carpeta del mod y el SDK Base.

Al abrir el mod, el menú principal usa el fondo de la calle (`materials/console/background01`). En 16:9 entra `background01_widescreen`. El icono pequeño de Steam sigue en `resource/icon.tga` y `logo.png`.

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

La stamina es independiente de la energía del traje HEV. Sprintar la gasta. Con hambre o sed en 0 no se regenera y el jugador recibe daño.

## Arsenal

Por defecto (`sv_survival_realistic_weapons 1`) el spawn y el `give` entregan pistola, revólver 357, SMG, escopeta, palanca y granada de fragmentación. Quedan fuera gravity gun, rifle de pulso, bola de energía, RPG, ballesta, stunstick y SLAM. `sv_survival_realistic_weapons 0` devuelve ese set.

## Día, noche y cajas

El reloj del HUD empieza a las 08:00. `sv_survival_day_length 720` es un día completo. De noche la pantalla se oscurece.

En Hammer (más adelante): `survival_crate` suelta comida y agua, `survival_bed` duerme. Para probar ya: `sv_cheats 1` y `ent_create survival_bed` o `survival_sleep`. Dormir llena la stamina y suma un poco de hambre y sed. Entre las 20:00 y las 07:00 el reloj salta a las 07:00.

Zombies de prueba (cheat): `survival_spawn_zombie` con `zombie`, `fast`, `poison` o `zombine`.

## Mapa de prueba

`mapcycle.txt` empieza por `dm_lockdown` y sigue con mapas de HL2MP que ya vienen en el SDK. El mapa de Paysandú sustituirá a `dm_lockdown` cuando exista.
