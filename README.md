# Crosshair Overlay

Aplicación ligera en C++ para mostrar una mirilla (*crosshair overlay*) en pantalla completa sobre Windows, con integración opcional para flujo de Fortnite desde el mismo ejecutable.

## ✨ Características

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

## 🛠️ Compilación

```bash
cmake -S . -B build
cmake --build build
```

> Este proyecto está orientado a Windows y requiere toolchain/SDK compatible con APIs Win32.

## ⚠️ Limitaciones actuales

- No existe persistencia de configuración (perfiles/archivo de settings).
- El soporte multimonitor es básico (usa dimensiones del monitor principal).
- La integración de Fortnite depende de nombre de proceso y protocolo estándar de Epic.
