// ------------------------------------------------
// Crosshair Overlay + Fortnite helper
// ------------------------------------------------

#define UNICODE
#define _UNICODE
#include <windows.h>
#include <d2d1.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <tlhelp32.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "shell32")

#define WM_TRAYICON (WM_USER + 1)
#define WM_OVERLAY_TOGGLE (WM_APP + 1)

#define ID_TRAY_STATUS 1000
#define ID_MODE_FORTNITE 1001
#define ID_MODE_ORIGINAL 1002
#define ID_LAUNCH_FORTNITE 1003
#define ID_TOGGLE_CROSSHAIR 1004
#define ID_TRAY_EXIT 1005

ID2D1Factory* pFactory = nullptr;
ID2D1HwndRenderTarget* pRenderTarget = nullptr;
ID2D1SolidColorBrush* pBrush = nullptr;
HINSTANCE gInstance = nullptr;
HWND gHwnd = nullptr;

const wchar_t CLASS_NAME[] = L"CrosshairOverlay";

struct CrosshairConfig {
    float length = 10.0f;
    float thickness = 1.0f;
    float gap = 0.0f;
    D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::Red);
};

struct AppConfig {
    int width = 1600;
    int height = 1200;
    int hz = 100;

    int originalWidth = 2560;
    int originalHeight = 1440;
    int originalHz = 100;

    std::wstring fortniteProtocol = L"com.epicgames.launcher://apps/Fortnite?action=launch";
    std::wstring fortniteProcessName = L"FortniteClient-Win64-Shipping.exe";
};

CrosshairConfig gCrosshair;
AppConfig gApp;
bool gCrosshairVisible = true;

std::vector<std::string> SplitArgs(const std::string& line) {
    std::vector<std::string> result;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        result.push_back(token);
    }
    return result;
}

bool ParseColor(const std::string& value, D2D1_COLOR_F& color) {
    std::istringstream iss(value);
    int r = 255, g = 0, b = 0;
    char sep1 = 0, sep2 = 0;
    if (!(iss >> r >> sep1 >> g >> sep2 >> b)) return false;
    if (sep1 != ',' || sep2 != ',') return false;

    r = std::clamp(r, 0, 255);
    g = std::clamp(g, 0, 255);
    b = std::clamp(b, 0, 255);
    color = D2D1::ColorF(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    return true;
}

void ShowNotification(const std::wstring& message) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = gHwnd;
    nid.uID = 1;
    nid.uFlags = NIF_INFO;
    wcscpy_s(nid.szInfoTitle, L"Fortnite Config");
    wcsncpy_s(nid.szInfo, message.c_str(), _TRUNCATE);
    nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

bool SetResolution(int width, int height, int hz) {
    DEVMODE mode = {};
    mode.dmSize = sizeof(mode);
    mode.dmPelsWidth = width;
    mode.dmPelsHeight = height;
    mode.dmDisplayFrequency = hz;
    mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

    return ChangeDisplaySettings(&mode, 0) == DISP_CHANGE_SUCCESSFUL;
}

struct ResolutionInfo {
    int width = 0;
    int height = 0;
    int hz = 0;
};

ResolutionInfo GetCurrentResolution() {
    ResolutionInfo info;
    info.width = GetSystemMetrics(SM_CXSCREEN);
    info.height = GetSystemMetrics(SM_CYSCREEN);

    DEVMODE current = {};
    current.dmSize = sizeof(current);
    if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &current)) {
        info.hz = static_cast<int>(current.dmDisplayFrequency);
    } else {
        info.hz = gApp.hz;
    }

    return info;
}

bool IsProcessRunning(const std::wstring& exeName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 entry = {};
    entry.dwSize = sizeof(entry);

    if (Process32First(snap, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, exeName.c_str()) == 0) {
                CloseHandle(snap);
                return true;
            }
        } while (Process32Next(snap, &entry));
    }

    CloseHandle(snap);
    return false;
}

void KillProcessByName(const std::wstring& exeName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32 entry = {};
    entry.dwSize = sizeof(entry);

    if (Process32First(snap, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, exeName.c_str()) == 0) {
                HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                if (process) {
                    TerminateProcess(process, 0);
                    CloseHandle(process);
                }
            }
        } while (Process32Next(snap, &entry));
    }

    CloseHandle(snap);
}

void AddTrayIcon(HWND hwnd) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"Fortnite Config + Crosshair");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void RemoveTrayIcon(HWND hwnd) {
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowTrayMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);

    ResolutionInfo current = GetCurrentResolution();
    std::wstringstream status;
    status << L"Resolución actual: " << current.width << L"x" << current.height << L" @ " << current.hz << L"Hz";

    HMENU hMenu = CreatePopupMenu();
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_GRAYED, ID_TRAY_STATUS, status.str().c_str());
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_MODE_FORTNITE, L"Activar modo Fortnite");
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_MODE_ORIGINAL, L"Restaurar resolución original");
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_LAUNCH_FORTNITE, L"Abrir Fortnite");
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TOGGLE_CROSSHAIR, gCrosshairVisible ? L"Ocultar Crosshair" : L"Mostrar Crosshair");
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TRAY_EXIT, L"Cerrar todo");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);
}

void RenderCrosshair(HWND hwnd) {
    if (!pRenderTarget) {
        RECT rc;
        GetClientRect(hwnd, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        pFactory->CreateHwndRenderTarget(
            rtProps,
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &pRenderTarget
        );

        pRenderTarget->CreateSolidColorBrush(gCrosshair.color, &pBrush);
    }

    pRenderTarget->BeginDraw();
    pRenderTarget->Clear(D2D1::ColorF(0, 0.0f));

    if (gCrosshairVisible) {
        D2D1_SIZE_F rtSize = pRenderTarget->GetSize();
        float cx = rtSize.width / 2;
        float cy = rtSize.height / 2;
        float len = gCrosshair.length;
        float thickness = gCrosshair.thickness;
        float gap = gCrosshair.gap;

        pRenderTarget->DrawLine(D2D1::Point2F(cx - len - gap, cy), D2D1::Point2F(cx - gap, cy), pBrush, thickness);
        pRenderTarget->DrawLine(D2D1::Point2F(cx + gap, cy), D2D1::Point2F(cx + len + gap, cy), pBrush, thickness);
        pRenderTarget->DrawLine(D2D1::Point2F(cx, cy - len - gap), D2D1::Point2F(cx, cy - gap), pBrush, thickness);
        pRenderTarget->DrawLine(D2D1::Point2F(cx, cy + gap), D2D1::Point2F(cx, cy + len + gap), pBrush, thickness);
    }

    pRenderTarget->EndDraw();
    ValidateRect(hwnd, nullptr);
}

void LaunchFortnite() {
    if (IsProcessRunning(gApp.fortniteProcessName)) {
        ShowNotification(L"Fortnite ya está ejecutándose.");
        return;
    }

    SetResolution(gApp.width, gApp.height, gApp.hz);
    ShellExecute(nullptr, L"open", gApp.fortniteProtocol.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    ShowNotification(L"Fortnite iniciado.");
}

void ShutdownAll() {
    KillProcessByName(gApp.fortniteProcessName);
    SetResolution(gApp.originalWidth, gApp.originalHeight, gApp.originalHz);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            AddTrayIcon(hwnd);
            break;

        case WM_PAINT:
        case WM_DISPLAYCHANGE:
            RenderCrosshair(hwnd);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu(hwnd);
            }
            break;

        case WM_OVERLAY_TOGGLE:
            gCrosshairVisible = !gCrosshairVisible;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_MODE_FORTNITE:
                    if (SetResolution(gApp.width, gApp.height, gApp.hz)) {
                        ShowNotification(L"Modo Fortnite activado.");
                    } else {
                        ShowNotification(L"No se pudo cambiar la resolución.");
                    }
                    break;

                case ID_MODE_ORIGINAL:
                    if (SetResolution(gApp.originalWidth, gApp.originalHeight, gApp.originalHz)) {
                        ShowNotification(L"Resolución original restaurada.");
                    } else {
                        ShowNotification(L"No se pudo restaurar la resolución.");
                    }
                    break;

                case ID_LAUNCH_FORTNITE:
                    LaunchFortnite();
                    break;

                case ID_TOGGLE_CROSSHAIR:
                    gCrosshairVisible = !gCrosshairVisible;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    ShowNotification(gCrosshairVisible ? L"Crosshair mostrado." : L"Crosshair oculto.");
                    break;

                case ID_TRAY_EXIT:
                    DestroyWindow(hwnd);
                    return 0;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            ShutdownAll();
            RemoveTrayIcon(hwnd);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int) {
    gInstance = hInstance;

    bool requestToggle = false;
    bool requestQuit = false;

    std::vector<std::string> args = SplitArgs(lpCmdLine ? lpCmdLine : "");
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "--toggle") {
            requestToggle = true;
        } else if (arg == "--quit") {
            requestQuit = true;
        } else if (arg == "--size" && i + 1 < args.size()) {
            gCrosshair.length = std::max(1.0f, static_cast<float>(atof(args[++i].c_str())));
        } else if (arg == "--thickness" && i + 1 < args.size()) {
            gCrosshair.thickness = std::max(1.0f, static_cast<float>(atof(args[++i].c_str())));
        } else if (arg == "--gap" && i + 1 < args.size()) {
            gCrosshair.gap = std::max(0.0f, static_cast<float>(atof(args[++i].c_str())));
        } else if (arg == "--color" && i + 1 < args.size()) {
            D2D1_COLOR_F color;
            if (ParseColor(args[++i], color)) {
                gCrosshair.color = color;
            }
        }
    }

    HWND existing = FindWindow(CLASS_NAME, nullptr);
    if (existing && requestQuit) {
        PostMessage(existing, WM_CLOSE, 0, 0);
        return 0;
    }

    if (existing && requestToggle) {
        PostMessage(existing, WM_OVERLAY_TOGGLE, 0, 0);
        return 0;
    }

    SetProcessDPIAware();
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_CROSS);
    RegisterClass(&wc);

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME, L"",
        WS_POPUP,
        0, 0, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    gHwnd = hwnd;

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    MARGINS margins = {-1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    ShowWindow(hwnd, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (pBrush) pBrush->Release();
    if (pRenderTarget) pRenderTarget->Release();
    if (pFactory) pFactory->Release();
    return 0;
}
