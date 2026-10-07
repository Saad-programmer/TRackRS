# Implementation Plan - Packaging Game Installers

To make the demo version of the game installable on any PC:
1. **Linux:** We will package the compiled binary, desktop shortcut, icons, and data files into a standard Debian package (`.deb`). This allows users on Ubuntu, Debian, Mint, etc., to install the game with a double-click.
2. **Windows:** We will write an NSIS (Nullsoft Scriptable Install System) script `installer.nsi`. Since the compilation environment is Linux and lacks Windows cross-compiler dependencies, this script will allow users to compile the final `.exe` installer on Windows.

---

## Proposed Changes

### Packaging Components

#### [NEW] [build_deb.sh](file:///home/alan/Downloads/Dev_Game/trackRS/build_deb.sh)
A script to automate the creation of the `.deb` package on Linux. It will:
1. Create the Debian package directory structure (control, postinst, etc.).
2. Copy the compiled binary `bin/trackrs` to `usr/local/games/`.
3. Copy the configuration template to `usr/local/games/`.
4. Copy the game's `data` directory to `usr/local/share/games/trackrs/`.
5. Copy the desktop menu entry to `usr/share/applications/` and icons to `usr/share/pixmaps/`.
6. Run `dpkg-deb --build` to produce the final `trackrs-demo_0.6.7_amd64.deb` installer.

#### [NEW] [installer.nsi](file:///home/alan/Downloads/Dev_Game/trackRS/installer.nsi)
A script for NSIS to build the Windows `.exe` installer. It will define:
1. The installation directory (e.g., `$PROGRAMFILES\TrackRS Demo`).
2. The list of files to copy (the game `.exe`, `.dll` dependencies, and the `data/` folder).
3. Creation of Start Menu and Desktop shortcuts.
4. An uninstaller script to cleanly remove files, shortcuts, and registry keys.

---

## Verification Plan

### Automated Checks
1. Run `build_deb.sh` to generate the `.deb` package:
   ```bash
   bash build_deb.sh
   ```
2. Verify that `trackrs-demo_0.6.7_amd64.deb` is successfully generated in the project root.

### Manual Verification
1. Inspect the contents of the generated `.deb` package:
   ```bash
   dpkg -c trackrs-demo_0.6.7_amd64.deb
   ```
2. Verify that the files are mapped to the correct paths:
   - `/usr/local/games/trackrs`
   - `/usr/local/share/games/trackrs/...`
   - `/usr/share/applications/trackrs.desktop`
