# Demon Slayer – Survival Game (v2)

## Controls
| Key | Action |
|-----|--------|
| UP  | Jump |
| DOWN | Duck |
| Z / X | Attack |
| P / TAB | Pause |
| ESC | Main Menu |
| R | Retry (on Game Over / Victory) |

## Level 1 – Survive the Demons
- **Demon 1** (ground) – Duck under or Attack to kill → +20 pts
- **Demon 2** (ground, stronger) – Attack to kill → +20 pts
- **Bird Demon** (air) – Duck under it or Attack → +15 pts
- **Obstacle** (ground, glowing) – **Jump over it** to collect Énergie → +50 pts
- Survive 40 seconds to advance to Level 2

## Level 2 – Dodge & Collect
- **Rocks** move very fast → **Jump over** them
- **Flying Demon 2** – **Duck under** or Attack
- **Money coins** (float) – Jump into them → +30 pts each, tracked as $
- After ~28 seconds, Akaza appears…

## Boss Fight – Akaza
- Tanjiro **auto-walks** toward Akaza
- Press **Z/X** to attack when close
- Akaza switches to hit texture when damaged
- Defeat all 10 HP → **Victory!**
- Background changes to **finale** and Nezuko appears

## Build (Windows / Linux)
```
mkdir build && cd build
cmake .. && cmake --build .
```
Requires SFML 2.6.
