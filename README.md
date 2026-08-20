# Lost In NUST

A top-down 2D adventure game built from scratch in **C++ and SFML**, following a new student (Yenna) trying to survive her first day at NUST — exploring the hostel, campus (C2), and SEECS, talking to NPCs, and clearing four different minigames to make it through the day.


## Features

- **Custom Tiled (.tmx) map renderer** built on [tmxlite](https://github.com/fallahn/tmxlite), with tileset batching, tile flip/rotation support, and per-tile visibility toggling (used for the book pickup)
- **3 connected maps** (Hostel → C2 → SEECS) with letterbox-style cutscene transitions between them
- **8-directional animated player and NPCs**, with idle/walk cycles, wandering AI, and collision-aware movement
- **Dialogue system** with typewriter text reveal and speech bubbles
- **Item pickup mechanic** (find and grab a book hidden in the hostel)
- **4 distinct minigames**, each tied to a different NPC:
  - Typing challenge (Student1)
  - General trivia quiz (Uni Boy)
  - Quick math challenge (Fem Std1)
  - Programming/OOP exam, IDE-styled (Uni Professor, SEECS)
- **Full game state machine**: menu, settings, character intro, free-roam, minigames, cutscenes, victory, and game over screens
- **Ending state**: clear all 4 minigames to trigger the "You Survived NUST" cutscene and victory screen

---

## Controls

| Key | Action |
|---|---|
| `W A S D` / Arrow keys | Move |
| `E` | Interact (talk to NPCs, pick up items, start minigames) |
| `Esc` | Leave a minigame early |
| `Enter` | Advance dialogue / confirm on result screens |
| Mouse | Menu navigation, settings, minigame multiple-choice answers |

---

## Tech Stack

- **Language:** C++
- **Graphics/Windowing:** [SFML 2.5+](https://www.sfml-dev.org/)
- **Tilemap parsing:** [tmxlite](https://github.com/fallahn/tmxlite)
- **Map editor:** [Tiled](https://www.mapeditor.org/)

---

## Project Structure

```
GAME/
├── MAIN-CODE/
│   └── MAINGAME_LOOP/
│       ├── main.cpp        # game loop, states, minigames
│       ├── main_char.cpp   # AnimatedPlayer class
│       └── npc.cpp         # NPCCharacter class (wandering AI, stationary NPCs)
├── MAPS/
│   └── Tiled.cpp           # TileMap class (tmxlite wrapper, collision rects)
├── Maps/                   # .tmx map files (Tiled projects)
├── Main-Char/               # player sprite animation frames
├── NPC/                     # NPC sprite animation frames
├── Buttons/                 # UI button sprites
├── BACKGROUND-IMAGE/        # menu background
├── Fonts/                   # bundled font(s)
└── third_party/tmxlite/     # tmxlite library
```

---

## Building & Running

**Dependencies:** SFML (graphics, window, system) and tmxlite must be installed and discoverable by the compiler/linker.

From `GAME/MAIN-CODE/MAINGAME_LOOP/`:

```bash
g++ main.cpp -o game -lsfml-graphics -lsfml-window -lsfml-system -ltmxlite
./game
```

> **Important:** the game must be run from `GAME/MAIN-CODE/MAINGAME_LOOP/` — all asset paths are relative to this folder (`../../Buttons`, `../../Maps`, etc.). Running the binary from anywhere else will fail to load textures/maps.

---

## Known Limitations / Roadmap

- Losing a minigame currently has no consequence — only clearing all 4 triggers the ending; there's no "fail" penalty yet
- Asset paths are relative and OS-path-separator dependent — not yet tested on Windows
- No audio/music implemented yet

---
