// ------------------------------------------------
// Crosshair Overlay + Fortnite helper + Settings UI
// ------------------------------------------------

#include <windows.h>
#include <d2d1.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <wchar.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "shell32")
#pragma comment(lib, "comdlg32")

#define WM_TRAYICON (WM_USER + 1)
#define WM_OVERLAY_TOGGLE (WM_APP + 1)

#define ID_TRAY_STATUS 1000
#define ID_MODE_FORTNITE 1001
#define ID_MODE_ORIGINAL 1002
#define ID_LAUNCH_FORTNITE 1003
#define ID_TOGGLE_CROSSHAIR 1004
#define ID_TRAY_EXIT 1005
#define ID_TRAY_SETTINGS 1006

#define ID_EDIT_LENGTH 2001
#define ID_EDIT_THICKNESS 2002
#define ID_EDIT_GAP 2003
#define ID_BTN_COLOR 2004
#define ID_COMBO_FORTNITE_RES 2005
#define IDI_APP_ICON 101

ID2D1Factory* pFactory = nullptr;
ID2D1HwndRenderTarget* pRenderTarget = nullptr;
ID2D1SolidColorBrush* pBrush = nullptr;
HINSTANCE gInstance = nullptr;
HWND gHwnd = nullptr;

const wchar_t CLASS_NAME[] = L"CrosshairOverlay";
const wchar_t SETTINGS_CLASS_NAME[] = L"CrosshairSettingsWindow";

struct CrosshairConfig {
    float length = 5.0f;
    float thickness = 1.0f;
    float gap = 2.0f;
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
COLORREF gCrosshairColorRef = RGB(255, 0, 0);

std::vector<std::string> SplitArgs(const std::string& line) {
    std::vector<std::string> result;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        result.push_back(token);
    }
    return result;
}

void ApplyCrosshairColorFromRef() {
    float r = static_cast<float>(GetRValue(gCrosshairColorRef)) / 255.0f;
    float g = static_cast<float>(GetGValue(gCrosshairColorRef)) / 255.0f;
    float b = static_cast<float>(GetBValue(gCrosshairColorRef)) / 255.0f;
    gCrosshair.color = D2D1::ColorF(r, g, b, 1.0f);

    if (pRenderTarget) {
        if (pBrush) {
            pBrush->Release();
            pBrush = nullptr;
        }
        pRenderTarget->CreateSolidColorBrush(gCrosshair.color, &pBrush);
    }
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
    gCrosshairColorRef = RGB(r, g, b);
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
    nid.hIcon = LoadIcon(gInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    if (!nid.hIcon) {
        nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }
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

bool GetPrimaryMonitorRect(RECT& outRect) {
    POINT origin = {0, 0};
    HMONITOR primaryMonitor = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfo(primaryMonitor, &mi)) return false;
    outRect = mi.rcMonitor;
    return true;
}

void SyncOverlayBounds(HWND hwnd) {
    if (!hwnd) return;

    RECT primaryRect = {};
    if (!GetPrimaryMonitorRect(primaryRect)) return;

    int left = primaryRect.left;
    int top = primaryRect.top;
    int width = primaryRect.right - primaryRect.left;
    int height = primaryRect.bottom - primaryRect.top;

    RECT rc = {};
    GetWindowRect(hwnd, &rc);
    int currentWidth = rc.right - rc.left;
    int currentHeight = rc.bottom - rc.top;

    if (rc.left != left || rc.top != top || currentWidth != width || currentHeight != height) {
        SetWindowPos(hwnd, HWND_TOPMOST, left, top, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    if (pRenderTarget) {
        pRenderTarget->Resize(D2D1::SizeU(static_cast<UINT32>(width), static_cast<UINT32>(height)));
    }

    InvalidateRect(hwnd, nullptr, FALSE);
}

void ShowTrayMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);

    ResolutionInfo current = GetCurrentResolution();
    std::wstringstream status;
    status << L"Current resolution: " << current.width << L"x" << current.height << L" @ " << current.hz << L"Hz";

    HMENU hMenu = CreatePopupMenu();
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_GRAYED, ID_TRAY_STATUS, status.str().c_str());
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_MODE_FORTNITE, L"Enable Fortnite mode");
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_MODE_ORIGINAL, L"Restore original resolution");
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_LAUNCH_FORTNITE, L"Launch Fortnite");
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TOGGLE_CROSSHAIR, gCrosshairVisible ? L"Hide crosshair" : L"Show crosshair");
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TRAY_SETTINGS, L"Settings");
    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TRAY_EXIT, L"Exit");

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
        float cx = 0.0f;
        float cy = 0.0f;
        RECT primaryRect = {};
        if (GetPrimaryMonitorRect(primaryRect)) {
            POINT centerScreen = {
                (primaryRect.left + primaryRect.right) / 2,
                (primaryRect.top + primaryRect.bottom) / 2
            };
            ScreenToClient(hwnd, &centerScreen);
            cx = static_cast<float>(centerScreen.x);
            cy = static_cast<float>(centerScreen.y);
        } else {
            D2D1_SIZE_F rtSize = pRenderTarget->GetSize();
            cx = rtSize.width / 2.0f;
            cy = rtSize.height / 2.0f;
        }
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
        ShowNotification(L"Fortnite is already running.");
        return;
    }

    SetResolution(gApp.width, gApp.height, gApp.hz);
    SyncOverlayBounds(gHwnd);
    ShellExecute(nullptr, L"open", gApp.fortniteProtocol.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    ShowNotification(L"Fortnite launched.");
}

void ShutdownAll() {
    KillProcessByName(gApp.fortniteProcessName);
    SetResolution(gApp.originalWidth, gApp.originalHeight, gApp.originalHz);
}

bool ParseResolutionText(const wchar_t* text, int& outWidth, int& outHeight) {
    if (!text) return false;
    int w = 0;
    int h = 0;
    if (swscanf_s(text, L"%dx%d", &w, &h) != 2) return false;
    if (w <= 0 || h <= 0) return false;
    outWidth = w;
    outHeight = h;
    return true;
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hLengthEdit, hThicknessEdit, hGapEdit, hFortniteResCombo;

    switch (msg) {
        case WM_CREATE: {
            CreateWindowW(L"STATIC", L"Length:", WS_VISIBLE | WS_CHILD,
                          10, 10, 70, 20, hwnd, nullptr, gInstance, nullptr);
            hLengthEdit = CreateWindowW(L"EDIT", nullptr,
                          WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                          90, 10, 120, 20, hwnd, (HMENU)ID_EDIT_LENGTH, gInstance, nullptr);

            CreateWindowW(L"STATIC", L"Thickness:", WS_VISIBLE | WS_CHILD,
                          10, 40, 70, 20, hwnd, nullptr, gInstance, nullptr);
            hThicknessEdit = CreateWindowW(L"EDIT", nullptr,
                          WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                          90, 40, 120, 20, hwnd, (HMENU)ID_EDIT_THICKNESS, gInstance, nullptr);

            CreateWindowW(L"STATIC", L"Gap:", WS_VISIBLE | WS_CHILD,
                          10, 70, 70, 20, hwnd, nullptr, gInstance, nullptr);
            hGapEdit = CreateWindowW(L"EDIT", nullptr,
                          WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                          90, 70, 120, 20, hwnd, (HMENU)ID_EDIT_GAP, gInstance, nullptr);

            CreateWindowW(L"BUTTON", L"Color", WS_VISIBLE | WS_CHILD,
                          10, 100, 70, 25, hwnd, (HMENU)ID_BTN_COLOR, gInstance, nullptr);

            CreateWindowW(L"STATIC", L"Fortnite res:", WS_VISIBLE | WS_CHILD,
                          10, 135, 80, 20, hwnd, nullptr, gInstance, nullptr);
            hFortniteResCombo = CreateWindowW(
                L"COMBOBOX",
                nullptr,
                WS_VISIBLE | WS_CHILD | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL,
                90, 132, 140, 120,
                hwnd,
                (HMENU)ID_COMBO_FORTNITE_RES,
                gInstance,
                nullptr
            );
            SendMessageW(hFortniteResCombo, CB_ADDSTRING, 0, (LPARAM)L"1600x1200");
            SendMessageW(hFortniteResCombo, CB_ADDSTRING, 0, (LPARAM)L"1440x1080");
            SendMessageW(hFortniteResCombo, CB_ADDSTRING, 0, (LPARAM)L"1720x1080");

            wchar_t resBuf[32];
            swprintf_s(resBuf, L"%dx%d", gApp.width, gApp.height);
            LRESULT selected = SendMessageW(hFortniteResCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)resBuf);
            if (selected == CB_ERR) {
                SendMessageW(hFortniteResCombo, CB_ADDSTRING, 0, (LPARAM)resBuf);
                selected = SendMessageW(hFortniteResCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)resBuf);
            }
            SendMessageW(hFortniteResCombo, CB_SETCURSEL, (WPARAM)selected, 0);

            CreateWindowW(L"BUTTON", L"OK", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                          90, 165, 55, 25, hwnd, (HMENU)IDOK, gInstance, nullptr);

            CreateWindowW(L"BUTTON", L"Cancel", WS_VISIBLE | WS_CHILD,
                          155, 165, 55, 25, hwnd, (HMENU)IDCANCEL, gInstance, nullptr);

            wchar_t buf[32];
            swprintf_s(buf, L"%.0f", gCrosshair.length);
            SetWindowTextW(hLengthEdit, buf);
            swprintf_s(buf, L"%.0f", gCrosshair.thickness);
            SetWindowTextW(hThicknessEdit, buf);
            swprintf_s(buf, L"%.0f", gCrosshair.gap);
            SetWindowTextW(hGapEdit, buf);
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_COLOR: {
                    CHOOSECOLOR cc = { sizeof(cc) };
                    COLORREF custom[16] = {};
                    cc.hwndOwner = hwnd;
                    cc.rgbResult = gCrosshairColorRef;
                    cc.lpCustColors = custom;
                    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
                    if (ChooseColor(&cc)) {
                        gCrosshairColorRef = cc.rgbResult;
                    }
                    break;
                }
                case IDOK: {
                    wchar_t buf[32];
                    GetWindowTextW(hLengthEdit, buf, 32);
                    gCrosshair.length = std::max(1.0f, static_cast<float>(_wtof(buf)));

                    GetWindowTextW(hThicknessEdit, buf, 32);
                    gCrosshair.thickness = std::max(1.0f, static_cast<float>(_wtof(buf)));

                    GetWindowTextW(hGapEdit, buf, 32);
                    gCrosshair.gap = std::max(0.0f, static_cast<float>(_wtof(buf)));

                    GetWindowTextW(hFortniteResCombo, buf, 32);
                    int selectedWidth = 0;
                    int selectedHeight = 0;
                    if (ParseResolutionText(buf, selectedWidth, selectedHeight)) {
                        gApp.width = selectedWidth;
                        gApp.height = selectedHeight;
                    }

                    ApplyCrosshairColorFromRef();
                    InvalidateRect(gHwnd, nullptr, FALSE);
                    DestroyWindow(hwnd);
                    break;
                }
                case IDCANCEL:
                    DestroyWindow(hwnd);
                    break;
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ShowSettingsWindow(HWND parent) {
    static bool classRegistered = false;

    if (!classRegistered) {
        WNDCLASS wc = {};
        wc.lpfnWndProc = SettingsProc;
        wc.hInstance = gInstance;
        wc.lpszClassName = SETTINGS_CLASS_NAME;
        RegisterClass(&wc);
        classRegistered = true;
    }

    HWND existing = FindWindow(SETTINGS_CLASS_NAME, nullptr);
    if (existing) {
        SetForegroundWindow(existing);
        return;
    }

    HWND hWnd = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        SETTINGS_CLASS_NAME,
        L"Crosshair Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 260, 235,
        parent, nullptr, gInstance, nullptr
    );

    ShowWindow(hWnd, SW_SHOW);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            AddTrayIcon(hwnd);
            SyncOverlayBounds(hwnd);
            break;

        case WM_PAINT:
        case WM_DISPLAYCHANGE:
            SyncOverlayBounds(hwnd);
            RenderCrosshair(hwnd);
            return 0;

        case WM_SIZE:
            if (pRenderTarget) {
                UINT width = static_cast<UINT>(LOWORD(lParam));
                UINT height = static_cast<UINT>(HIWORD(lParam));
                if (width > 0 && height > 0) {
                    pRenderTarget->Resize(D2D1::SizeU(width, height));
                }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
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
                        SyncOverlayBounds(hwnd);
                        ShowNotification(L"Fortnite mode enabled.");
                    } else {
                        ShowNotification(L"Failed to change resolution.");
                    }
                    break;

                case ID_MODE_ORIGINAL:
                    if (SetResolution(gApp.originalWidth, gApp.originalHeight, gApp.originalHz)) {
                        SyncOverlayBounds(hwnd);
                        ShowNotification(L"Original resolution restored.");
                    } else {
                        ShowNotification(L"Failed to restore resolution.");
                    }
                    break;

                case ID_LAUNCH_FORTNITE:
                    LaunchFortnite();
                    break;

                case ID_TOGGLE_CROSSHAIR:
                    gCrosshairVisible = !gCrosshairVisible;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    ShowNotification(gCrosshairVisible ? L"Crosshair shown." : L"Crosshair hidden.");
                    break;

                case ID_TRAY_SETTINGS:
                    ShowSettingsWindow(hwnd);
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

    ResolutionInfo startupResolution = GetCurrentResolution();
    gApp.originalWidth = startupResolution.width;
    gApp.originalHeight = startupResolution.height;
    gApp.originalHz = startupResolution.hz;

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

    RECT primaryRect = {};
    int left = 0;
    int top = 0;
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    if (GetPrimaryMonitorRect(primaryRect)) {
        left = primaryRect.left;
        top = primaryRect.top;
        width = primaryRect.right - primaryRect.left;
        height = primaryRect.bottom - primaryRect.top;
    }

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME, L"",
        WS_POPUP,
        left, top, width, height,
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
