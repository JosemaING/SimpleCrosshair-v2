# Crosshair Overlay
Tired of a simple overlay taking over 100MB of RAM for no reason? </br> A very light, fast and portable crosshair overlay built with C++. </br> The current version is as minimal as it can get

### Crosshair Customization
UI was included in the ```gui-incluided``` branch </br>
Windows system tray (or notification area) -> Select CrosshairOverlay -> Settings

### CPU/RAM usage
- Executable Size: **13.5KB**.
- CPU Usage: **0%** (YES really)
- RAM Usage: **9.1 MB**

### How to quit the app?
Windows system tray (or notification area) -> Select CrosshairOverlay -> Exit

---

## Project overview

This repository contains a minimal Win32 crosshair overlay implemented in C++.

### Current implementation

- Transparent, click-through fullscreen overlay window.
- Always-on-top rendering layer.
- Crosshair drawn at screen center using Direct2D.
- System tray icon with a context menu option to exit.
- Unicode build configuration through CMake.

### Technical details

- Main source file: `overlay.cpp`.
- Build system: `CMakeLists.txt`.
- APIs/libraries in use:
  - Win32 API (`windows.h`, window lifecycle, tray integration)
  - Direct2D (`d2d1`) for drawing
  - DWM (`dwmapi`) for transparent frame extension
  - Shell API (`shell32`) for tray icon operations

### Known limitations in current codebase

- No profile system or settings persistence.
- No multi-monitor targeting logic beyond primary screen dimensions.
- Fortnite integration uses protocol launch + process-name termination and assumes standard process naming.



### Fortnite integration (single executable)

The same executable now includes tray actions for:

- Switching to Fortnite resolution mode (`1600x1200 @ 100Hz` by default).
- Restoring original resolution (`2560x1440 @ 100Hz` by default).
- Launching Fortnite through Epic protocol URI.
- Showing/hiding the built-in crosshair overlay.
- Closing everything from one menu action (kills Fortnite and restores original resolution).

### Command-line options

The executable supports basic arguments for external automation:

- `--size <value>`: crosshair arm length.
- `--thickness <value>`: line thickness.
- `--gap <value>`: center gap.
- `--color R,G,B`: crosshair color in RGB (0-255).
- `--toggle`: toggles visibility on a running instance.
- `--quit`: requests graceful shutdown of a running instance.

Example:

```powershell
crosshair.exe --size 12 --thickness 2 --gap 3 --color 0,255,0
```

### Build

```bash
cmake -S . -B build
cmake --build build
```

> Note: This project targets Windows APIs and requires a Windows-compatible toolchain/SDK.
