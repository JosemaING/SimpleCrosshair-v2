# Crosshair Overlay

Aplicación ligera en C++ para mostrar una mirilla (*crosshair overlay*) en pantalla completa sobre Windows, con integración opcional para flujo de Fortnite desde el mismo ejecutable.

## ✨ Características

- Ventana transparente y *click-through* (no bloquea clics del juego).
- Overlay siempre visible por encima de otras ventanas (*always on top*).
- Renderizado de mirilla centrada con Direct2D.
- Icono en bandeja del sistema con menú contextual.
- Control de visibilidad del crosshair en tiempo de ejecución.
- Opciones CLI para ajustar tamaño, grosor, separación y color.
- Acciones integradas para Fortnite (resolución, lanzamiento y cierre global).

## 🧱 Tecnologías usadas

- **Win32 API** (`windows.h`) para ciclo de vida de ventana y sistema.
- **Direct2D** (`d2d1`) para dibujado de la mirilla.
- **DWM** (`dwmapi`) para extensión de marco transparente.
- **Shell API** (`shell32`) para icono y notificaciones de bandeja.
- **Tool Help API** (`tlhelp32`) para enumeración y cierre de procesos.

## 📁 Estructura del repositorio

- `overlay.cpp`: implementación principal de la aplicación.
- `CMakeLists.txt`: configuración de compilación.
- `README.md`: documentación del proyecto.

## 🎮 Integración Fortnite (mismo ejecutable)

Desde el menú de bandeja se puede:

1. **Activar modo Fortnite** (`1600x1200 @ 100Hz`, configurable en código).
2. **Restaurar resolución original** (`2560x1440 @ 100Hz`, configurable en código).
3. **Abrir Fortnite** mediante protocolo de Epic Launcher.
4. **Mostrar/Ocultar crosshair**.
5. **Cerrar todo** (cierra Fortnite y restaura resolución original).

## ⌨️ Opciones de línea de comandos

| Opción | Descripción |
|---|---|
| `--size <valor>` | Longitud de los brazos de la mirilla. |
| `--thickness <valor>` | Grosor de la línea. |
| `--gap <valor>` | Separación central de la mirilla. |
| `--color R,G,B` | Color RGB (0-255), por ejemplo `0,255,0`. |
| `--toggle` | Alterna visibilidad en una instancia ya abierta. |
| `--quit` | Cierra de forma remota una instancia ya abierta. |

### Ejemplo

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
