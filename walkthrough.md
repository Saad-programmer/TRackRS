# Walkthrough - Demo Mode Lock & Installers

We successfully implemented a restricted demo version of the game by locking all game events, practice races, and other single race maps except for the longest one, **"Ahead"**. We then packaged the demo into installable formats for both Linux and Windows.

---

## 1. Demo Mode Lock (Changes Made)

### Menu and Race Selection Locking
Modified [menu.cpp](file:///home/alan/Downloads/Dev_Game/trackRS/src/Trigger/menu.cpp) in the following areas:
- **Main Menu Options:** The "events" and "practice" options are now rendered as `Weak` labels with `(locked)` appended, and they are no longer clickable.
- **Single Race List:** All maps other than `"Ahead"` in the race list are displayed as `Weak` labels with ` (locked)` appended, and are not clickable.
- **Race Prep Next/Prev:** Locked next/prev buttons on the level preparation screen if the target level is not `"Ahead"`.
- **Keyboard Navigation:** Disallowed arrow keys from switching to other levels on the level prep screen unless the target level is `"Ahead"`.
- **Action Guards:** Added checks in `levelScreenAction` under `AA_PICK_LVL` and `AA_START_LVL` to guarantee that only the `"Ahead"` level can be selected or started.

---

## 2. Installers & Packaging

### Linux Installer (Debian Package)
Created a script `build_deb.sh` to package the compiled executable, configuration definitions, desktop shortcuts, icons, and game assets into a standard `.deb` installer.
- **Output Package:** `trackrs-demo_0.6.7_amd64.deb`
- **Installation Path:** `/usr/local/games/trackrs` with data files in `/usr/local/share/games/trackrs`
- **Menu Shortcut:** Adds a system-wide application shortcut with icon `trackrs.png` in `/usr/share/pixmaps`.

### Windows Installer (NSIS Script)
Created an NSIS installer script `installer.nsi` to generate a standalone Windows installer `.exe`.
- **Script Functionality:** Defines the installation directory, packages the Windows `.exe` and `.dll` dependencies, bundles the `data/` assets directory recursively, adds registry entries for clean Add/Remove Programs integration, and sets up Desktop/Start Menu shortcuts.

---

## How to Install and Run the Demo

### On Linux (Debian, Ubuntu, Mint, Pop!_OS)
To install the packaged demo game, run the following command or double-click the `.deb` file:
```bash
sudo dpkg -i trackrs-demo_0.6.7_amd64.deb
sudo apt-get install -f   # If there are any missing dependency libraries
```
Once installed, the game can be launched via the application menu or directly in the terminal:
```bash
trackrs
```

### On Windows
To compile the installer `.exe` for Windows:
1. Ensure the Windows C++ compiled binary `trackrs.exe` and its dependency `.dll` files are in the `bin/` folder.
2. Compile `installer.nsi` using the free [NSIS Utility (makensis)](https://nsis.sourceforge.io/).
3. This generates `trackrs-demo-setup.exe` which can be executed on any Windows machine to install and play the demo.
