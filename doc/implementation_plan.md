# Add Video Intro to Game Start

This plan outlines the implementation of a video intro sequence that plays immediately when the game is launched, before the main application window opens, matching the presentation of professional games.

## User Review Required

> [!IMPORTANT]
> The intro video `inVideo.mp4` will be played using `ffplay` synchronously before SDL initialization. Users can skip the video at any time by pressing **ESC** or **Q** on their keyboard.

## Proposed Changes

---

### Source Code modifications

#### [MODIFY] [main.cpp](file:///home/alan/Downloads/trigger-rally-code-r1032/src/Trigger/main.cpp)
- **`main()`**:
  - Implement a `fileExists` helper using `<fstream>` to search for `inVideo.mp4` in `.` (current directory), `bin/inVideo.mp4`, and `../inVideo.mp4`.
  - If the file is found, execute `ffplay -fs -autoexit -noborder -loglevel quiet <path>` using `std::system(...)` to play the video in fullscreen mode.
  - After the video completes (or is skipped), proceed to initialize `MainApp` and start the game normally.

---

## Verification Plan

### Automated Tests
- Run `make` in `src/` to ensure clean compilation.

### Manual Verification
- Run `./bin/trackrs` from the bin directory or `./trackrs` from the repository root.
- Verify that the video starts playing in fullscreen immediately.
- Press **ESC** or **Q** to verify that the video is skippable and that the game successfully starts afterwards.
- Let the video play to the end to verify that it automatically exits and launches the game.
