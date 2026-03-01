# Crosshair Overlay + Fortnite Helper

Aplicacion Win32 en C++ para mostrar una mira (`crosshair overlay`) transparente en Windows, con control desde la bandeja del sistema y funciones para flujo rapido de Fortnite.

## Que hace la aplicacion

- Crea un overlay fullscreen transparente y click-through.
- Dibuja una mira con Direct2D.
- Mantiene la mira centrada en el monitor principal.
- Permite cambiar resolucion para modo Fortnite y restaurar resolucion original.
- Lanza Fortnite por protocolo de Epic.

## Caracteristicas principales

- Overlay `topmost`, transparente y sin capturar clics.
- Icono personalizado en ejecutable y bandeja (`fnite.ico`).
- Ventana `Settings` con configuracion de mira y resolucion Fortnite.
- Deteccion automatica de resolucion original al iniciar.
- Argumentos CLI para automatizacion.

## Valores por defecto

### Mira

- `length = 5.0`
- `thickness = 1.0`
- `gap = 2.0`
- color inicial: rojo

### Modo Fortnite

- Resolucion por defecto: `1600x1200`
- Frecuencia usada por el modo Fortnite: `100 Hz`
- Presets disponibles en `Settings`:
- `1600x1200` (predeterminada)
- `1440x1080`
- `1720x1080`

## Menu de bandeja

Click derecho en el icono de la app en el area de notificacion:

- `Enable Fortnite mode`
- `Restore original resolution`
- `Launch Fortnite`
- `Hide/Show crosshair`
- `Settings`
- `Exit`

## Ventana Settings

Permite configurar:

- `Length`
- `Thickness`
- `Gap`
- `Color`
- `Fortnite res` (selector de resolucion)

Al pulsar `OK`, los cambios se aplican al instante.

## Resolucion original

Al iniciar la app se detecta automaticamente la resolucion actual del sistema (`ancho`, `alto`, `Hz`) y se guarda como resolucion original. Esa resolucion es la que usa `Restore original resolution`.

## Argumentos CLI

Puedes lanzar el ejecutable con:

- `--size <valor>`
- `--thickness <valor>`
- `--gap <valor>`
- `--color R,G,B`
- `--toggle`
- `--quit`

Ejemplo:

```powershell
.\build\bin\CrosshairOverlay.exe --size 12 --thickness 2 --gap 3 --color 0,255,0
```

## Requisitos

- Windows 10/11
- CMake >= 3.10
- Toolchain C++ para Windows:
- MinGW-w64 (MSYS2) o
- Visual Studio 2022 (Desktop development with C++)

## Compilacion

### Opcion A: MinGW (MSYS2)

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --config Release
```

### Opcion B: Visual Studio 2022

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release
```

Nota: usa carpetas de build distintas por generador (`build` y `build-vs`) para evitar conflictos de cache de CMake.

## Ejecucion

MinGW:

```powershell
.\build\bin\CrosshairOverlay.exe
```

Visual Studio:

```powershell
.\build-vs\bin\Release\CrosshairOverlay.exe
```

## Estructura del repositorio

- `overlay.cpp`: logica principal (overlay, rendering, tray, settings, Fortnite helper).
- `CMakeLists.txt`: configuracion de build.
- `icon.rc`: recurso de Windows para incrustar icono en el `.exe`.
- `fnite.ico`: icono de la aplicacion.
- `LICENSE`: licencia del proyecto.

## Solucion de problemas

- Error al compilar `cannot open output file ... Permission denied`:
- cierra `CrosshairOverlay.exe` y vuelve a compilar.
- Error de CMake por mezcla de generadores (`MinGW`/`Visual Studio`):
- usa otra carpeta de build o limpia `CMakeCache.txt` y `CMakeFiles`.
- No ves el icono/menu:
- revisa tambien el area de iconos ocultos de Windows.

## Limitaciones actuales

- No hay persistencia de settings en archivo (se reinician al cerrar).
- La integracion de Fortnite depende de protocolo de Epic y nombre de proceso.
- La frecuencia para modo Fortnite esta fija en `100 Hz` salvo cambios en codigo.
