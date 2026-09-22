# Polygon Perihelion
Fly and curve your way through a treacherous asteroid belt! Pilot your ship between hurdling boulders and shoot down pesky meteoroids. How long will you last? 

**Polygon Perihelion** is a 2D arcade space shooter built in C++ using SFML

## Features

* 2D arcade-style space combat
* Dynamic asteroid spawning and difficulty progression
* Multiple projectile types
* Temporary power-ups
* Health and shield systems
* Animated explosions
* Energy system with regeneration
* Sound effects
* Customizable keybinds
* Audio controls
* High score system
* Hitbox mode

## Controls

Controls can be customized in-game through the **Keybinds** menu. Open it by pausing and selecting the keybinds option.

### Default Controls

| Key         | Action               |
| ----------- | -------------------- |
| `Enter`     | Select / Confirm     |
| `Esc`       | Back / Pause         |
| `←` / `→`   | Rotate               |
| `↑`         | Shoot                |
| `Shift + ↑` | Alternate projectile |
| `` ` ``     | Toggle hitboxes      |

## Power-Ups

Power-ups are temporary rather than permanent upgrades, keeping each run focused on adapting to what is happening on screen.

Power-ups include:

* **Shield** — Provides additional protection from incoming asteroid collisions.
* **Agility** — Temporarily improves movement.
* **Rapid Fire** — Temporarily increases firing speed.

## Difficulty

The asteroid field becomes progressively more difficult as the score increases.

* Asteroid spawn frequency
* Asteroid toughness

## Tech Stack

* C++11
* SFML 2.6.1
  - SFML Graphics
  - SFML Window
  - SFML System
  - SFML Audio

### Mac

Example compile command:

```bash
g++ -std=c++11 main.cpp -I/opt/homebrew/Cellar/sfml/2.6.1/include \
-L/opt/homebrew/Cellar/sfml/2.6.1/lib \
-lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio \
-o PolygonPerihelion
```

Then run:

```bash
./PolygonPerihelion
```

### Windows

A pre-built Windows version can be distributed with the required SFML runtime files and game assets (check artifacts).
No C++ compiler is required to run the compiled version.

## Development

Polygon Perihelion was built as a personal C++ project to explore game development, real-time systems, collision detection, UI design, audio, and gameplay balancing.

The project was developed without a game engine, with the game's core systems implemented directly using C++ and SFML.

## How To Play

Use the rotation keys (left and right arrows by default) to move around turn the rocket away from dangerous asteroids. Smaller lighter boulders will devastate your hull, while regular to large or medium to dark ones will take you out instantly. Use the fire key (up arrow by default) to fire basic projectiles. These shots can be fired quickly and do okay damage. Press the fire key in conjunction with the modifier key (lshift by default) to fire a heavy projectile. These shots can't be fired very fast but deal high damage. Careful using either too carelessly, as using projectiles consumes energy, and you may run dry. However, energy replenishes automatically over time. Destroy asteroids to get temporary powerups that help you survive longer. Tougher asteroids have higher chances of dropping powerups, but are more difficult to destroy and not guaranteed. 

## Screenshots

<img width="798" height="798" alt="Screenshot 2026-09-22 at 4 48 51 PM" src="https://github.com/user-attachments/assets/f1639355-fe47-4ad1-a2ed-0ee8fc8b7419" />

<img width="798" height="798" alt="Screenshot 2026-09-22 at 5 40 55 PM" src="https://github.com/user-attachments/assets/152eb25b-c441-4e38-8cf5-b26757006d75" />

<img width="798" height="798" alt="Screenshot 2026-09-22 at 5 41 30 PM" src="https://github.com/user-attachments/assets/17406235-be08-434a-aeb0-5e5fc509f441" />

<img width="798" height="798" alt="Screenshot 2026-09-22 at 5 26 47 PM" src="https://github.com/user-attachments/assets/f30b27e5-ba67-4921-9e78-7c0e89834b6b" />



