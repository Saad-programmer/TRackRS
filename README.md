<div align="center">

# 🏁 TrackRS

### A fast-paced, single-player 3D rally racing game written in C++ / OpenGL

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](doc/COPYING.txt)
![Language](https://img.shields.io/badge/language-C%2B%2B11-00599C.svg)
![Graphics](https://img.shields.io/badge/graphics-OpenGL%20%2B%20GLEW-5586A4.svg)
![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20Windows-lightgrey.svg)
![Version](https://img.shields.io/badge/version-0.6.7-orange.svg)
![Maps](https://img.shields.io/badge/maps-132%20standalone%20%2B%2097%20event%20stages-brightgreen.svg)

**Drift. Listen to your co-driver. Beat the clock.**

*Creator: Saad AIT YAHIA — [@Saad-programmer](https://github.com/Saad-programmer)*

</div>

---

## 📖 Table of Contents

1. [About the Project](#-about-the-project)
2. [Feature Overview](#-feature-overview)
3. [Gameplay](#-gameplay)
4. [Controls](#-controls)
5. [Quick Start](#-quick-start)
6. [Building from Source](#-building-from-source)
7. [Installers & Packaging](#-installers--packaging)
8. [Configuration Reference](#-configuration-reference)
9. [Stereo 3D / VR-Style Rendering](#-stereo-3d-support)
10. [Repository Structure](#-repository-structure)
11. [Architecture](#-architecture)
12. [Game Content](#-game-content)
13. [Content Creation Guide](#-content-creation-guide)
14. [Demo Mode](#-demo-mode)
15. [UI, Intro Video & Music](#-ui-intro-video--music)
16. [Troubleshooting](#-troubleshooting)
17. [Version History](#-version-history)
18. [Contributing](#-contributing)
19. [License & Credits](#-license--credits)

---

## 🎯 About the Project

**TrackRS** is a 3D rally racing simulation. You race against the clock through long point-to-point stages, guided by a spoken **co-driver** who reads pace notes ("*medium right into easy left*"). The game has a physics engine built around drifting, and terrain that changes how the car behaves: dirt, gravel, tarmac, sand, snow, ice, mud and water. Weather, fog and lighting change from stage to stage.

TrackRS is a **rebranded and modernised fork of the open-source rally game Trigger Rally**. It adds:

- A new identity (executable, config folders, metadata, desktop entry, docs all renamed to *TrackRS*).
- An arcade-style animated menu (see [UI, Intro Video & Music](#-ui-intro-video--music)).
- A fullscreen intro video and looping menu music.
- A **demo mode** build and ready-made installers for Linux (`.deb`) and Windows (NSIS).

The project is licensed under the **GNU GPL v2** (see [License & Credits](#-license--credits)).

---

## ✨ Feature Overview

| Area | What you get |
|---|---|
| **Racing** | Timed point-to-point stages with checkpoints, target times, optional multi-lap races, a time penalty for off-road driving, and a "recover" system. |
| **Physics** | Custom rigid-body vehicle simulation (`PSim`): per-wheel suspension, engine power curves, gearbox, drag/lift, per-terrain friction, wheel sinking, and body damage. |
| **Terrain** | Heightmap-based terrain with colour map, foliage map, road map and terrain-type map; a rigidity table decides which sprites (trees, signs) are solid. |
| **Co-driver** | Spoken pace notes (several voices) plus on-screen pace-note icons in multiple sign sets. |
| **Content** | 16 events, 132 standalone single-race maps, 97 event stages, 9 vehicles (3 manufacturers × 3 classes), and 25 downloadable plugin packs. |
| **Progression** | Unlockable events and vehicles; best-time tables saved per player profile. |
| **Ghost car** | Record your run and race against your best lap (opt-in setting). |
| **Weather** | Rain, snow (point / square / textured flakes), fog, scrolling clouds, per-level lighting. |
| **Cameras** | Chase, bumper, side, hood and periscope views, with a rotatable chase camera. |
| **Input** | Keyboard (fully remappable), joystick, joypad, wheel & pedals. |
| **Stereo 3D** | Quad-buffer hardware stereo plus red-blue, red-green, red-cyan and yellow-blue anaglyph. |
| **Audio** | OpenAL/ALUT sound: engine, wind, gravel, gear shifts, crashes, co-driver voice, menu music. |
| **Storage** | PhysFS virtual filesystem lets data be read from folders or zip archives. |

---

## 🏎️ Gameplay

### Game modes

The main menu offers:

| Mode | Description |
|---|---|
| **Events** | A championship of several single races. You must finish every stage in time to win the event, which **unlocks the next event and extra cars**. |
| **Practice** | Practise the stages of an event you have access to, without the progression pressure. |
| **Single Race** | Pick any single map from the list and race it. Multi-page list. |
| **Options / Controls** | Player name, video, audio, key and joystick bindings. |
| **Best Times** | Per-level time tables, sortable by player name, car name, car class and total time. |

### Race flow

1. **Choose a level** and read its description, author, comment and minimap on the preparation screen.
2. **Choose a vehicle** from the level's `vehicleoption` list. The selection screen shows the car's real values.
3. **Countdown → Racing → Finished.** The game moves through these three states. The in-race OSD shows the speedometer (kph or mph, optionally with a digital readout), the course time and a map.
4. **Checkpoints** are driven through in order. The course time freezes briefly as you pass one.
5. Finish before the **target time** to pass. Your time is saved to your player profile, and your ghost can be recorded.

### Vehicles

Nine vehicles ship with the game: three models in three classes.

| Class | Cordo | Evo | Fox |
|---|---|---|---|
| **Kit-Car** | `cordo_kitcar` | `evo_kitcar` | `fox_kitcar` |
| **Super 500** | `cordo_super500` | `evo_super500` | `fox_super500` |
| **WRC** | `cordo_wrc` | `evo_wrc` | `fox_wrc` |

The tuning differs by model. From the 0.6.6 changelog: *"Fox offroad, Evo circuit, Cordo in between."* Each car is defined by a `.vehicle` XML file giving its mass, engine power curve, gearbox, wheels (drive, steering, brakes, suspension force and damping, friction) and body clip points for collision.

---

## 🎮 Controls

### Default keyboard bindings

| Key | Action |
|---|---|
| `↑` | Accelerate |
| `↓` | Foot brake / reverse |
| `←` / `→` | Steer left / right |
| `Space` | Handbrake |
| `R` | Recover car |
| `Q` | Recover car at the previous checkpoint |
| `P` | Pause race |
| `C` | Change camera view |
| `,` / `.` | Rotate camera (third-person view) |
| `M` | Toggle map |
| `N` | Toggle on-screen display / UI |
| `K` | Toggle checkpoints |
| `F12` | Take a screenshot (saved to your user folder) |

All bindings can be changed in the config file or the in-game **Controls** menu. SDL key names are listed at <https://wiki.libsdl.org/SDL_Keycode>.

### Joystick / wheel / gamepad

Joystick support is **disabled by default**. Enable it in the `<controls>` section of the config. The default config ships two templates:

- **Wheel & pedals / stick** — axis 1 for throttle/brake, axis 0 for steering, button 0 for the handbrake.
- **Gamepad** — buttons 0/1 for throttle/brake, axis 0 for steering, button 2 for the handbrake.

Each axis supports a `deadzone` and a `maxrange`.

---

## 🚀 Quick Start

### Linux (from source, fastest path)

```bash
# 1. Install dependencies (Debian/Ubuntu/Mint)
sudo apt install build-essential libsdl2-dev libsdl2-image-dev libglew-dev \
                 libopenal-dev libalut-dev libtinyxml2-dev libphysfs-dev \
                 libgl1-mesa-dev libglu1-mesa-dev

# 2. Build
cd src
make

# 3. Run (the game runs straight from the build tree, no install needed)
cd ../bin
./trackrs
```

### Linux (from the `.deb` installer)

```bash
sudo dpkg -i trackrs-demo_0.6.7_amd64.deb
sudo apt-get install -f      # pulls in any missing libraries
trackrs                      # or launch from your application menu
```

### Windows

Build `trackrs.exe` with MSYS2 (see [Windows build](#windows)), then compile `installer.nsi` with NSIS to produce `trackrs-demo-setup.exe`.

---

## 🛠️ Building from Source

### Requirements

- A C++11 compiler (g++ 4.9 or newer recommended) and binutils
- GNU Make
- Development libraries:

| Library | Purpose | Debian / Ubuntu package | Fedora / RPM package |
|---|---|---|---|
| GL | OpenGL rendering | `libgl1-mesa-dev` | `mesa-libGL-devel` |
| GLU | OpenGL utilities | `libglu1-mesa-dev` | `mesa-libGLU-devel` |
| GLEW | OpenGL extension loading | `libglew-dev` | `glew-devel` |
| OpenAL | 3D audio | `libopenal-dev` | `openal-soft-devel` |
| ALUT | OpenAL utility toolkit | `libalut-dev` | `freealut-devel` |
| PhysFS | Virtual filesystem | `libphysfs-dev` | `physfs-devel` |
| SDL2 | Window, input, events | `libsdl2-dev` | `SDL2-devel` + `SDL2-static` |
| SDL2_image | PNG / JPG loading | `libsdl2-image-dev` | `SDL2_image-devel` |
| TinyXML-2 | XML parsing | `libtinyxml2-dev` | `tinyxml2-devel` |

**Optional runtime tool:** `ffplay` (part of FFmpeg), used to play the intro video if `inVideo.mp4` is present.

### Linux

```bash
cd src
make                 # build the executable into ../bin/trackrs
make install         # as root: installs binary, data, docs, desktop entry, appdata
make uninstall       # removes everything `install` placed
make clean           # removes object files, dependency files, executable, backups
make dist            # creates a tarball + MD5 sum (directory must be named trackrs-<version>)
```

The `GNUmakefile` compiles with `-std=c++11 -Wall -Wextra -pedantic`, defines `-DNDEBUG -DUNIX -DPACKAGE_VERSION="0.6.7"`, and links `-lSDL2main -lGL -lGLEW -lSDL2 -lSDL2_image -lphysfs -lopenal -lalut -lpthread -ltinyxml2`. It compiles every `.cpp` in `src/PEngine`, `src/PSim` and `src/Trigger`.

#### Build variables

Override any of these on the command line:

| Variable | Default | Meaning |
|---|---|---|
| `OPTIMS` | `-march=native -mtune=native -Ofast` | Optimisation flags. Use more conservative values when packaging. |
| `WARNINGS` | `-Wall -Wextra -pedantic` | Warning flags. |
| `prefix` | `/usr/local` | Install prefix. |
| `exec_prefix` | `$(prefix)` | Executable prefix (binary goes to `$(exec_prefix)/games`). |
| `DESTDIR` | *(empty)* | Staged-install root, for packagers. |

```bash
OPTIMS="-O0 -g" make                         # debug build
prefix=/usr exec_prefix=/usr make install    # system-wide install
DESTDIR="$HOME/TR_staged" make install       # staged install
```

#### Developer build

Define `INDEVEL` to turn on terrain information and co-driver checkpoint visuals, which release builds hide:

```bash
cd src
OPTIMS="-DINDEVEL" make
```

#### Default data paths

At startup the game searches the `<datadirectory>` list in `bin/trackrs.config.defs`, in this order:

1. `../data` (relative to the binary)
2. `C:\Program Files\TrackRS\data`
3. `/usr/share/games/trackrs`
4. `/usr/local/share/games/trackrs`

If none is valid, the game will not start. Packagers should edit this list. The config file must stay in the same folder as the binary.

### Windows

Windows builds use MSYS2 and the makefiles `src/GNUmakefile.MSYS` (32-bit) and `src/GNUmakefile.MSYS64` (64-bit).

| Software | Link |
|---|---|
| MSYS2 | <https://www.msys2.org/> |
| CMake | <https://cmake.org/download/> |
| TrackRS build scripts | <https://sourceforge.net/projects/trigger-rally/files/devkit/build_scripts/> |

You also need the Windows development libraries: GLEW, PhysFS, SDL2, SDL2_image, TinyXML-2, libjpeg, libpng, zlib and the FMOD Studio API 1.06.xx. Read the `build_readme.txt` shipped with the build scripts. Visual Studio is not officially supported.

Toolchain versions used for the last documented Windows release (0.6.6.1): MSYS2 20180531, GCC 8.2.1 (64-bit) / 7.4.0 (32-bit), CMake 3.13.4, NSIS 3.04, GLEW 2.1.0, SDL2 2.0.9, SDL2_image 2.0.4, TinyXML-2 7.0.1, libjpeg 9c, libpng 1.6.36, PhysFS 3.0.1, zlib 1.2.11. The 32-bit binary was built with `OPTIMS="-march=i686 -mtune=generic -O2" make build`.

More detail lives in [`doc/BUILDING.txt`](doc/BUILDING.txt).

---

## 📦 Installers & Packaging

### Debian package (`build_deb.sh`)

```bash
bash build_deb.sh      # run from the repository root, after building bin/trackrs
```

Produces `trackrs-demo_0.6.7_amd64.deb`.

| Item | Installed to |
|---|---|
| Executable | `/usr/local/games/trackrs` |
| Default config | `/usr/local/games/trackrs.config.defs` |
| Game data | `/usr/local/share/games/trackrs/` |
| Menu shortcut | `/usr/share/applications/trackrs.desktop` (its `Exec=` is rewritten to the absolute path) |
| Icon | `/usr/share/pixmaps/trackrs.png` |

Declared dependencies: `libsdl2-2.0-0`, `libsdl2-image-2.0-0`, `libglew2.x`, `libopenal1`, `libalut0`, `libphysfs1`, `libtinyxml2-*`.

Inspect the result with `dpkg -c trackrs-demo_0.6.7_amd64.deb`.

### Windows installer (`installer.nsi`)

An NSIS (Nullsoft Scriptable Install System) script using Modern UI 2.

1. Put `trackrs.exe` and its `.dll` files in `bin/`.
2. Compile with `makensis installer.nsi` on Windows.
3. Output: `trackrs-demo-setup.exe`.

The installer installs to `C:\Program Files\TrackRS Demo`, bundles `data\` recursively, writes Add/Remove Programs registry entries, creates Start Menu and Desktop shortcuts, and includes an uninstaller.

### Other packaging notes

- The `make install` target supports `DESTDIR` staged installs.
- The Windows release stores game data in an uncompressed `data.zip`, which loads faster.
- AppStream metadata is in `data/metainfo/trackrs.appdata.xml` and the desktop entry in `data/metainfo/trackrs.desktop`.

---

## ⚙️ Configuration Reference

On first launch, TrackRS creates a per-user folder and copies the defaults into it:

| OS | Location |
|---|---|
| Linux | `~/.trackrs/trackrs-<version>.config` |
| Windows | `C:\Users\<You>\Application Data\trackrs-team\trackrs\` |

This folder also holds your player profile, best times and screenshots. **To reset everything, delete the config file.** It is re-created on the next launch.

The defaults come from [`bin/trackrs.config.defs`](bin/trackrs.config.defs):

### `<player>`

| Attribute | Default | Notes |
|---|---|---|
| `name` | `Player` | Letters, digits, `_` and space only. Other characters may stop the profile loading. |
| `copydefplayers` | `yes` | Copy default players from `data/defplayers/` if missing. |
| `skipsaves` | `4` | Saves skipped before writing the profile. `0` = save after every finished race; `-1` = save only at exit (**progress is lost if the game crashes**). |

### `<video>`

| Attribute | Default | Notes |
|---|---|---|
| `automatic` | `yes` | `yes` = fullscreen at desktop resolution; `no` = use the values below. |
| `width`, `height` | `800`, `600` | Used when `automatic="no"`. |
| `bpp` | `0` | Colour depth. |
| `fullscreen` | `no` | |
| `requirergb`, `requirealpha`, `requiredepth`, `requirestencil` | `no`, `no`, `yes`, `no` | GL buffer requirements. |
| `stereo` | `none` | `none`, `quadbuffer`, `red-blue`, `red-green`, `red-cyan`, `yellow-blue`. |
| `stereoeyeseparation` | `0.07` | Eye distance in metres. |
| `stereoswapeyes` | `no` | Set `yes` if the image looks swapped. |

### `<audio>`

Volumes range from `0` to `1`.

| Attribute | Default | Controls |
|---|---|---|
| `enginevolume` | `0.2` | Car engine gain |
| `sfxvolume` | `0.7` | Wind, skids, crashes, gear changes |
| `codrivervolume` | `1.0` | Co-driver voice |

### `<graphics>`

| Attribute | Default | Values / notes |
|---|---|---|
| `anisotropy` | `4` | Power of two (`1`…`16`), `off`, or `max`. |
| `foliage` | `yes` | Bushes, grass and trees. |
| `roadsigns` | `yes` | Road signs and sprites. |
| `weather` | `yes` | Rain and snow. |
| `snowflaketype` | `textured` | `point` (fast), `square` (failsafe), `textured` (fancy). |
| `dirteffect` | `yes` | Dirt thrown by the wheels. May reduce the frame rate. |

### `<datadirectory>`

Ordered list of `<data path="…"/>` candidates. See [Default data paths](#default-data-paths).

### `<parameters>`

| Attribute | Default | Notes |
|---|---|---|
| `drivingassist` | `0.33` | Driving assistance strength. |
| `enablesound` | `yes` | |
| `enablecodriversigns` | `yes` | On-screen pace-note icons. |
| `speedunit` | `mph` | `mph` or `kph`. |
| `codriver` | `ab` | Voice folder under `data/sounds/codriver/` (`ab`, `paula`, `tim`). Use `mime` to disable the voice. |
| `codriversigns` | `plain` | Icon set under `data/textures/CodriverSigns/` (e.g. `abon`, `glossy`, `plain`, `white`). |
| `codriversignslife` | `3.0` | Seconds before the signs start fading. |
| `codriversignsposx`, `codriversignsposy` | `0.0`, `0.45` | Centre of the signs; `(0, 0)` is the screen centre. |
| `codriversignsscale` | `0.2` | `1.0` covers the whole screen. |
| `enablefps` | `no` | Show the frame rate. |
| `enableghost` | `no` | Record and replay ghost cars. |

### `<controls>`

`<keyboard>`, plus any number of `<joystick>` blocks. Actions: `forward`, `back`, `left`, `right`, `handbrake`, `recover`, `recoveratcheckpoint`, `cammode`, `camleft`, `camright`, `showmap`, `pauserace`, `showui`, `showcheckpoint`, `next`.

### Menu colours

`data/menu.colors` sets the RGBA colour (0–1) of every widget class: `normal`, `click`, `hover`, `listnormal`, `listclick`, `listhover`, `weak`, `strong`, `marked`, `header`, `bnormal`, `bclick`, `bhover`. The shipped palette is steel-silver text with neon-orange highlights.

---

## 🕶️ Stereo 3D Support

TrackRS can render in stereo for 3D / VR-style viewing. The settings are in the `<video>` section (see above).

| Mode | Hardware needed |
|---|---|
| `quadbuffer` | A card with hardware stereo support (Nvidia Quadro, ATI FireGL), shutter glasses and a sync doubler. Needs `Option "Stereo"` in the Xorg `Device` section. CRTs at 75–85 Hz or more work best. |
| `red-cyan`, `red-blue`, `red-green`, `yellow-blue` | Matching anaglyph glasses. **Works on any system.** `red-cyan` and `yellow-blue` give the best performance. |

Auto-stereo LCDs are also supported.

> ⚠️ **Health warning:** If you are prone to motion sickness, or have a condition such as epilepsy, do not use stereo mode.

Full guide: [`doc/README-stereo.txt`](doc/README-stereo.txt).

---

## 🗂️ Repository Structure

```
trackrs/
├── bin/
│   └── trackrs.config.defs        # Default configuration template (must sit next to the binary)
├── data/                          # All game assets (read through PhysFS)
│   ├── events/                    # 16 events, each with its stage levels
│   │   └── 01-triggercup/ …  16-infinite/
│   │       ├── <event>.event      # Event definition (name, author, lock state, unlocks, level list)
│   │       └── <level>/           # <level>.level + h/c/f/r/t/mm/ss images
│   ├── maps/                      # 132 standalone single-race maps
│   ├── plugins/                   # 25 optional map/event packs (.zip)
│   ├── vehicles/                  # 9 cars (.vehicle + .obj + .mtl, plus wheel models)
│   ├── sounds/                    # Engine, wind, gravel, crash, gear, intro music; codriver/{ab,paula,tim}/
│   ├── textures/                  # CodriverSigns, roadsigns, sky, splash, structures, veg, water, intro_frames
│   ├── icon/                      # App icons (PNG sizes 16–256, SVG, Windows .ico)
│   ├── metainfo/                  # trackrs.desktop, trackrs.appdata.xml
│   ├── menu.colors                # Menu colour palette
│   └── rigidity.xml               # Solidity of sprites (trees, signs, bushes)
├── diagrams/
│   ├── Architecture-Complete-class.puml   # PlantUML: all three modules
│   └── Logique_du_Jeu.puml                # PlantUML: game-logic classes
├── doc/                           # README.txt, BUILDING.txt, README-win.txt, README-stereo.txt,
│                                  # COPYING.txt, DATA_AUTHORS.txt, walkthrough notes, find_longest_map.py
├── src/
│   ├── include/                   # All headers (.h)
│   ├── PEngine/                   # Rendering, audio, terrain, textures, models, config, utilities
│   ├── PSim/                      # Physics simulation
│   ├── Trigger/                   # Game logic, menus, controls, ghost, options, main
│   ├── GNUmakefile                # Linux build
│   ├── GNUmakefile.MSYS           # Windows 32-bit (MSYS2)
│   └── GNUmakefile.MSYS64         # Windows 64-bit (MSYS2)
├── build_deb.sh                   # Debian package builder
├── installer.nsi                  # Windows NSIS installer script
└── .gitignore
```

---

## 🧩 Architecture

The C++ code is split into three layers. Each depends only on the ones below it.

```
┌───────────────────────────────────────────────────────────┐
│  Trigger  (src/Trigger)   — game logic & user interface   │
│  main · game · menu · control · option · ghost · render   │
├───────────────────────────────────────────────────────────┤
│  PSim     (src/PSim)      — physics simulation            │
│  sim · rigidbody · vehicle · engine · collision · damage  │
├───────────────────────────────────────────────────────────┤
│  PEngine  (src/PEngine)   — rendering, audio, assets      │
│  app · render · terrain · texture · model · vbuffer ·     │
│  fxman · audio · config · rigidity · physfs_rw · util ·   │
│  vmath                                                    │
└───────────────────────────────────────────────────────────┘
        External: SDL2 · OpenGL/GLEW · OpenAL/ALUT · PhysFS · TinyXML-2
```

Full PlantUML class diagrams are in [`diagrams/`](diagrams/).

### `PEngine` — rendering, audio & assets

| File | Responsibility |
|---|---|
| `app.cpp` | Application base: SDL window, GL context, main loop, input events. |
| `render.cpp` | Scene rendering helpers and camera. |
| `terrain.cpp` | Loads heightmap, colour, foliage, road and terrain-type maps; builds the mesh with LOD; answers height and terrain-type queries. |
| `texture.cpp` | Texture loading, mipmaps and anisotropic filtering. |
| `model.cpp` | Wavefront `.obj` loader (optimised in 0.6.6). |
| `vbuffer.cpp` | OpenGL vertex-buffer wrapper. |
| `fxman.cpp` | Particle and effect manager: rain, snow, dirt, dust, smoke. |
| `audio.cpp` | OpenAL device, sources, buffers and listener. |
| `config.cpp` | Reads and writes the XML config via TinyXML-2. |
| `rigidity.cpp` | Loads `rigidity.xml` so sprite collisions are known. |
| `physfs_rw.cpp` | Bridges PhysFS files into SDL's `RWops`. |
| `util.cpp`, `vmath.cpp` | Helpers; vector, quaternion and matrix maths. |

### `PSim` — physics

| File | Responsibility |
|---|---|
| `sim.cpp` | The physics world: bodies, gravity, air density, fixed time step. |
| `rigidbody.cpp` | Rigid body with mass, inertia tensor, force and torque integration. |
| `vehicle.cpp` | Vehicle model: parts, wheels, suspension, drive system, controls. Core types: `car`, `tank`, `helicopter`, `plane`, `hovercraft`. |
| `engine.cpp` | Engine RPM and torque from the power curve, gearbox. |
| `collision.cpp` | Ray, sphere, box and triangle intersection, plus collision response. |
| `damage.cpp` | Body damage that degrades performance. |

Wheel/ground contact is computed along the wheel plane rather than straight down, and wheels sink differently into different terrain types.

### `Trigger` — game & UI

| File | Responsibility |
|---|---|
| `main.cpp` | Entry point and `MainApp`. It runs the intro video, then the state machine: `AS_LOAD_1…3`, `AS_LEVEL_SCREEN`, `AS_CHOOSE_VEHICLE`, `AS_IN_GAME`, `AS_END_SCREEN`. |
| `game.cpp` | `TriggerGame`: checkpoints, co-driver checkpoints, countdown/racing/finished, timers, vehicles. |
| `menu.cpp` | Menu GUI and event/level progress logic. |
| `control.cpp` | Maps keyboard and joystick input to vehicle controls. |
| `option.cpp` | Options screens. |
| `ghost.cpp` | `PGhost`: records position, orientation and wheels, then replays them. |
| `render.cpp` | In-race HUD: speedometer, map, OSD, co-driver signs. |

### Key game concepts

- **`Gamestate`**: `countdown` → `racing` → `finished`. **`Gamefinish`**: `not_finished` / `pass` / `fail`.
- **`CameraMode`**: `chase`, `bumper`, `side`, `hood`, `periscope`.
- **`CheckPoint`** is a world coordinate. **`CodriverCP`** is a coordinate plus the spoken/printed note.
- **Menus** are identified by `AM_*` constants (top, events, practice, single race, times, options, controls) and **actions** by `AA_*` constants.

---

## 🌍 Game Content

### Events (championships)

Finish every stage of an event to unlock the next one. Only the first starts unlocked.

| # | Event | Author |
|---|---|---|
| 01 | Trigger Cup | Richard Langridge |
| 02 | Alps' Trophy | Onsemeliot |
| 03 | High Stakes | Onsemeliot |
| 04 | Chasing Shadows | Onsemeliot |
| 05 | Weird Dare | Onsemeliot |
| 06 | Picturesque Tournament | Onsemeliot |
| 07 | Odd Coincidence | Onsemeliot |
| 08 | Coy Goblet | Onsemeliot |
| 09 | Marathon | Onsemeliot |
| 10 | Flood | Onsemeliot |
| 11 | Tight Trophy | Onsemeliot |
| 12 | Encountered Challenges | Onsemeliot |
| 13 | Generic Dare | Onsemeliot |
| 14 | Smooth Globe | Onsemeliot |
| 15 | Rough Voyage | Onsemeliot |
| 16 | Infinite Dare | Onsemeliot |

### Single-race maps

132 standalone maps ship in `data/maps/`, from `aegyptian` to `zone`. Examples: *Ahead* (a very long, fast, hilly gravel stage), *Frozen*, *Glacier*, *Jupiter*, *Sandstorm*, *Sunset*, *Swamp*, *Xmas*, *Zampano*.

### Plugins

`data/plugins/` holds 25 optional packs, installed by copying a `.zip` into the folder: `event-rscup`, `event-westernchallenge`, and maps such as `banana`, `crossmountain`, `delta`, `desertrush`, `ghats`, `greengrounds`, `helicoil`, `icypeak`, `labrally`, `marsspirit`, `monza`, `mountainclimbing`, `mountainpass`, `pistol`, `pulp`, `roundhouse`, `santa`, `serpentine`, `snowlevel`, `snowyhills`, `tea`, `tobago`, `volcanic`.

### Co-driver

Three voice sets (`ab`, `paula`, `tim`) cover note words (left, right, hairpin, square, chicane, easy, medium, hard, flat, long, cut, don't cut, jump, over, into, finish) and surfaces (tarmac, gravel, dirt, grass, sand, snow, ice, mud, water). The `ab` voice also has composed phrases such as `LeftLongCutInto`. Matching icon sets (`abon`, `glossy`, `plain`, `white`) are in `data/textures/CodriverSigns/`.

---

## 🛣️ Content Creation Guide

TrackRS is moddable using ordinary image and text editors. Content is defined in XML.

### Level files (`.level`)

A level describes its terrain, weather, allowed vehicles and race. The excerpt below comes from the shipped *Ahead* map:

```xml
<level name="Ahead" comment="Pateince!" author="Onsemeliot"
       description="very long, fast and hilly on gravel"
       screenshot="ss.png" minimap="mm.png">

  <terrain heightmap="h.png" colormap="c.png" hudmap="c.png"
           foliagemap="f.png" roadmap="r.png" terrainmap="t.png"
           tilesize="64" horizontalscale="4" verticalscale="15">
    <blurfilter> <row data="0.002 0.002 0.002 0.002" /> … </blurfilter>
    <foliageband middle="0.4" range="0.39" density="0.000006"
                 sprite="/textures/veg/dry-slim-bush1.png" spritecount="2" scale="2" />
    …
  </terrain>

  <weather cloudtexture="/textures/sky/light-coluds.jpg" cloudscrollrate="0"
           fogcolor="0.9, 0.95, 1" fogdensity="0.001" fogdensitysky="1" />

  <vehicleoption type="/vehicles/evo/evo.vehicle" />

  <race coordscale="4, 4" targettime="1500">
    <startposition pos="-156, 992" oridegrees="270" />
    <checkpoint coords="-134, 995" notes="flat right into flat left" />
    …
  </race>
</level>
```

| Image | Role |
|---|---|
| `h.png` | **Heightmap.** Use PNG; JPG artefacts change the geometry. |
| `c.png` | Colour map (terrain texture colours). |
| `f.png` | Foliage map (optional). |
| `r.png` | Road map (optional). |
| `t.png` | Terrain-type map: dirt, tarmac, ice… (optional). |
| `mm.png` | Minimap shown in menus. |
| `ss.png` | Screenshot shown in menus. |

Key ideas:

- **`foliageband`** places sprites by height band (`middle` ± `range`) at a given `density`.
- **`race`**: `targettime` in seconds, `coordscale` converts checkpoint coordinates to world units, `laps` makes it a multi-lap race.
- **`checkpoint notes`** drive the co-driver: words like *easy, medium, hard, flat, square, hairpin, chicane, long, cut, into, over*.
- **`rigidity.xml`** says how solid each sprite is (trees `1.0`, bushes about `0.005`, grass about `0.001`).

### Event files (`.event`)

```xml
<event name="01. Trigger Cup" author="Richard Langridge" locked="no">
  <unlocks file="/events/02-alps/alps.event" />
  <level file="warmup/warmup.level" />
  <level file="operation/operation.level" />
  …
</event>
```

### Vehicle files (`.vehicle`) and 3D models

```xml
<vehicle name="Cordo" class="Kit-Car" type="car">
  <genparams mass="870.0" wheelmodel="cordo_kitcar_wheel.obj" wheelscale="0.033" />
  <drivesystem>
    <engine powerscale="18000">
      <powerpoint rpm="2000" power="0.4" /> …
    </engine>
    <gearbox> <gear absolute="0.047" /> <gear relative="1.52" /> … </gearbox>
  </drivesystem>
  <part name="body" model="cordo_kitcar.obj" scale="0.01" …>
    <wheel drive="1.0" steer="0.85" brake1="200.0" pos="0.63, 0.87, -0.17"
           radius="0.3" force="45000.0" dampening="20000.0" friction="0.018"/>
    …
    <clip type="body" pos="1.0, 1.5, 1.0" force="300000.0" dampening="30000.0" />
  </part>
</vehicle>
```

Rules for `.obj` models:

- One material per `.obj`. Only the texture it references is used.
- All faces must be **triangles**.
- Everything except the wheels must be one object. Wheels are in separate files.
- Blender export settings that work: *Apply Modifiers, Include Normals, Include Edges, Write Materials, Triangulate Faces, Objects as OBJ Objects*.

To test a model, point an existing `.vehicle` at your file under `data/vehicles/<name>/`.

### Car textures

The textures were authored as SVG in Inkscape. To re-render them you need these free fonts: TeX Gyre Heros, Roboto, Bowlby One, URW Gothic L.

### Helper script

`doc/find_longest_map.py` walks `data/`, parses every `.level`, and prints the top 10 by target time and by route length (summed checkpoint distances). It was used to pick the longest map for the demo. Edit its hard-coded `maps_dir` path before running it.

---

## 🔒 Demo Mode

The current `menu.cpp` is a **restricted demo**:

- The **Events** and **Practice** entries on the main menu are shown as weak `(locked)` labels and cannot be clicked.
- In the **Single Race** list, every map except **"Ahead"** is shown as `(locked)`.
- The next/previous buttons and arrow-key navigation on the level-preparation screen are blocked unless the target is "Ahead".
- `levelScreenAction` guards `AA_PICK_LVL` and `AA_START_LVL`, so only "Ahead" can be selected or started.

To build a full-content version, remove these guards in `src/Trigger/menu.cpp`. The installers (`trackrs-demo_0.6.7_amd64.deb`, `trackrs-demo-setup.exe`) are named for the demo.

---

## 🎨 UI, Intro Video & Music

### Intro video

If `inVideo.mp4` is found in `.`, `bin/` or `..`, the game plays it **before** opening the main window, using:

```
ffplay -fs -autoexit -noborder -loglevel quiet <path>
```

Press **ESC** or **Q** to skip. If the file or `ffplay` is missing, the game starts normally. The video file is excluded from the repository by `.gitignore`.

### Menu music

`data/sounds/intro.wav` (16-bit PCM mono, for native OpenAL/ALUT compatibility) loops in the menus. It starts at launch, pauses during a race, and resumes after a race ends or is aborted.

### Arcade-style menu

- **Ken Burns** pan/zoom on the splash background.
- **Animated gradient** overlay from deep navy to glowing gold/red.
- **CRT scanlines** crawling across the background.
- **Particles:** 40 orange sparks and white dust streaks.
- **Button micro-animations:** a hovered button slides right by up to 12 px, an angled neon racing-stripe slides in behind it, and pulsing `>` `<` chevrons appear.
- **Pulsing neon-orange TrackRS logo** with drop shadow and double underline; section headers get double underlines.
- **Glassmorphic HUD panels** with pulsing orange borders.
- **Colour palette:** steel-silver text and neon-orange highlights (`data/menu.colors`).

---

## 🩺 Troubleshooting

| Problem | What to try |
|---|---|
| Game won't start / "data directory not found" | Check `<datadirectory>` paths in the config. The first valid path wins. |
| Odd behaviour after editing the config | Delete `~/.trackrs/trackrs-*.config` to reset to defaults. |
| Player profile not loading | Use only letters, digits, `_` and space in the player name. |
| Low frame rate | Set `dirteffect="no"`, `weather="no"`, `foliage="no"`, lower `anisotropy`, or use `snowflaketype="point"`. |
| Snow looks broken | Try `snowflaketype="square"`. |
| No intro video | Check `inVideo.mp4` is present and `ffplay` is installed. |
| Windows crash | Run `trackrs.RUNLOG.cmd` from the `bin` folder. It writes `trackrs.log` to `%TEMP%`. |
| Stereo image swapped | Set `stereoswapeyes="yes"`. |
| Stereo uncomfortable | Lower `stereoeyeseparation`. |

---

## 📜 Version History

| Version | Date | Highlights |
|---|---|---|
| **0.6.7** | — | TrackRS rebrand, arcade menu, intro video & music, demo mode, `.deb` and NSIS installers. |
| 0.6.6.1 | 04/03/2019 | Windows binaries, data optimised for release, internal TinyXML-2 removed. |
| 0.6.6 | 01/02/2019 | Physics code reorganised; vehicle retuning; wheel sinking; wheel-plane ground contact; new font; 2 events and 20 races (36 maps); more vegetation. |
| 0.6.5 | 18/12/2016 | Co-driver frame-rate fix on Windows; 2 events and 13 races; road-sign option; moved to TinyXML-2, SDL2, SDL2_image; 64-bit Windows. |
| 0.6.4 | 23/04/2016 | Best-time records; unlockable vehicles and events; Pause and Recover-at-Checkpoint keys; multi-lap option; off-road time penalty; native fullscreen default. |
| 0.6.3 | 30/01/2016 | Menu and OSD improvements; co-driver on 75% of maps; terrain physics tweaks; many new maps. |
| 0.6.2 | 05/05/2015 | Windows fixes and binaries. |
| 0.6.1 | 25/10/2014 | `.obj` textures; 6 events and 23 races; FOSS media replacements and new icon. |
| 0.6.0 | 08/10/2011 | Practice mode; paging on single-races screen; kph/mph; hybrid speedometer. |
| 0.5.x | 2004–2010 | Stereo support, ARB multitexture, screenshots, RAII refactor, joystick deadzones. |
| 0.4.x | early releases (dates in `doc/README.txt` are inconsistent) | PhysFS, text config, GLEW, GPL licence, per-level weather, camera rotate. |

The full log, including the 0.4.x/0.5.x dates, is in [`doc/README.txt`](doc/README.txt).

---

## 🤝 Contributing

Contributions are welcome.

1. **Fork** the repository and create a feature branch.
2. Keep to the existing C++11 style, and make sure the build is warning-clean under `-Wall -Wextra -pedantic`.
3. For new content, follow the [Content Creation Guide](#-content-creation-guide) and add authorship and licence info to `doc/DATA_AUTHORS.txt`.
4. Test with `make` in `src/`, then run `bin/trackrs`.
5. Open a **pull request** describing what changed and why.

Ideas that would help: a CMake build, more translations, more maps and vehicles, a Windows CI build, and automated tests for the physics module.

---

## ⚖️ License & Credits

### Code

Released under the **GNU General Public License v2 or later**. See [`doc/COPYING.txt`](doc/COPYING.txt) or <https://www.gnu.org/licenses/gpl-2.0.html>.

### Data

Files under `data/` have **different authors and licences** (GPL, CC0, CC-BY 3.0, CC-BY-SA 3.0, Pixabay). See [`doc/DATA_AUTHORS.txt`](doc/DATA_AUTHORS.txt) before redistributing.

### Credits

- **TrackRS creator & maintainer:** Saad AIT YAHIA — [@Saad-programmer](https://github.com/Saad-programmer)
- **Original project:** *Trigger Rally*, on which TrackRS is based, and its contributors, including map authors **Onsemeliot**, **Andrei Bondor** and **Richard Langridge**, plus the many others listed in `DATA_AUTHORS.txt`.
- **Libraries:** SDL2, SDL2_image, GLEW, OpenAL, ALUT, PhysFS, TinyXML-2, and FMOD (Windows builds).

### Links

- Repository: <https://github.com/Saad-programmer/trackrs>
- Windows plugin downloads and forums (inherited from upstream): <https://sourceforge.net/projects/trigger-rally/>

---

<div align="center">

**Happy racing! 🏁**

</div>
