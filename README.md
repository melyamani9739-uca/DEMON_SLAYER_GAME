# 🎮 Demon Slayer – Survival Game (v2)

A 2D survival game inspired by *Demon Slayer*, developed in C++ using object-oriented programming.  
The player controls Tanjiro, fights demons, avoids obstacles, and faces the boss Akaza.

---

## 🎮 Controls

| Key        | Action                         |
|------------|--------------------------------|
| ↑ (UP)     | Jump                           |
| ↓ (DOWN)   | Duck                           |
| Z / X      | Attack                         |
| P / TAB    | Pause                          |
| ESC        | Main Menu                      |
| R          | Retry (Game Over / Victory)    |

---

## 🧩 Gameplay

### 🟢 Level 1 – Survive the Demons
- Demon 1 (ground): Duck or Attack → +20 pts  
- Demon 2 (stronger): Attack → +20 pts  
- Bird Demon (air): Duck or Attack → +15 pts  
- Obstacle (glowing): Jump → +50 énergie  

🎯 **Goal:** Survive 40 seconds to reach Level 2  

---

### 🔵 Level 2 – Dodge & Collect
- Fast rocks → Jump over them  
- Flying Demon → Duck or Attack  
- Coins → Jump to collect → +30$ each  

⏱ After ~28 seconds → **Boss appears**

---

### 🔴 Boss Fight – Akaza
- Tanjiro automatically moves toward Akaza  
- Press Z/X to attack when close  
- Akaza changes texture when damaged  
- Defeat all 10 HP → Victory  

✨ **Final Scene:**
- Background changes  
- Nezuko appears  

---

## ⚙️ Build & Run (Windows / Linux)

```bash
mkdir build
cd build
cmake ..
cmake --build .

## 👩‍💻 Author
MANAL EL YAMANI
