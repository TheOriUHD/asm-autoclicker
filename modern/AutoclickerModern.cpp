#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <cwchar>
#include <windowsx.h>
#include <shellapi.h>

#include "resource.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "shell32.lib")

namespace {
constexpr int IDC_CPS = 101;
constexpr int IDC_LEFT = 102;
constexpr int IDC_RIGHT = 103;
constexpr int IDC_START = 104;
constexpr int IDC_EXIT = 105;
constexpr int IDC_STATUS = 106;
constexpr int IDC_TITLE = 107;
constexpr int IDC_SUBTITLE = 108;
constexpr int IDC_CPS_LABEL = 109;
constexpr int IDC_BUTTON_LABEL = 110;
constexpr int IDC_CPS_SLIDER = 112;
constexpr int IDC_COPYRIGHT = 113;
constexpr int HOTKEY_START = 200;
constexpr int HOTKEY_STOP = 201;
constexpr int MAX_CPS = 600;
constexpr UINT STATUS_TIMER = 301;
constexpr wchar_t CPS_SLIDER_CLASS[] = L"KantiCpsSlider";

enum class StatusKind { Ready, Running, Error };

HWND g_hwnd = nullptr;
HWND g_title = nullptr;
HWND g_subtitle = nullptr;
HWND g_copyright = nullptr;
HWND g_cpsLabel = nullptr;
HWND g_buttonLabel = nullptr;
HWND g_cpsEdit = nullptr;
HWND g_cpsSlider = nullptr;
HWND g_leftRadio = nullptr;
HWND g_rightRadio = nullptr;
HWND g_startButton = nullptr;
HWND g_exitButton = nullptr;
HWND g_status = nullptr;
HFONT g_titleFont = nullptr;
HFONT g_textFont = nullptr;
HBRUSH g_bgBrush = nullptr;
HBRUSH g_editBrush = nullptr;

volatile LONG g_running = 0;
volatile LONG g_stopRequested = 0;
volatile LONG g_skipCountdown = 0;
int g_cps = 12;
bool g_rightClick = false;
bool g_darkMode = false;
bool g_syncingCps = false;
bool g_draggingSlider = false;
bool g_statusPulse = false;
StatusKind g_statusKind = StatusKind::Ready;
LARGE_INTEGER g_freq{};

COLORREF BgColor() { return g_darkMode ? RGB(32, 32, 36) : RGB(255, 255, 255); }
COLORREF TextColor() { return g_darkMode ? RGB(242, 242, 247) : RGB(0, 0, 0); }
COLORREF MutedTextColor() { return g_darkMode ? RGB(180, 180, 188) : RGB(88, 88, 92); }
COLORREF EditBgColor() { return g_darkMode ? RGB(48, 48, 54) : RGB(255, 255, 255); }
COLORREF AccentColor() { return g_darkMode ? RGB(96, 165, 250) : RGB(0, 95, 184); }
COLORREF SliderTrackColor() { return g_darkMode ? RGB(70, 70, 78) : RGB(225, 225, 229); }
COLORREF SliderThumbColor() { return g_darkMode ? RGB(245, 245, 247) : RGB(255, 255, 255); }
COLORREF StatusColor() {
    if (g_statusKind == StatusKind::Ready) return RGB(34, 197, 94);
    if (g_statusKind == StatusKind::Error) return RGB(239, 68, 68);
    return g_statusPulse ? RGB(168, 85, 247) : RGB(126, 34, 206);
}

int Scale(int value) {
    const UINT dpi = g_hwnd ? GetDpiForWindow(g_hwnd) : 96;
    return MulDiv(value, static_cast<int>(dpi), 96);
}

int ClampCps(int value) {
    if (value < 1) return 1;
    if (value > MAX_CPS) return MAX_CPS;
    return value;
}

void SetCpsValue(int value, bool updateEdit = true) {
    g_cps = ClampCps(value);
    if (updateEdit && g_cpsEdit) {
        wchar_t text[16]{};
        swprintf_s(text, L"%d", g_cps);
        g_syncingCps = true;
        SetWindowTextW(g_cpsEdit, text);
        g_syncingCps = false;
        RedrawWindow(g_cpsEdit, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
    }
    if (g_cpsSlider) InvalidateRect(g_cpsSlider, nullptr, TRUE);
}

int SliderXFromValue(HWND hwnd) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const int left = Scale(12);
    const int right = rc.right - Scale(12);
    return left + MulDiv(g_cps - 1, right - left, MAX_CPS - 1);
}

int SliderValueFromX(HWND hwnd, int x) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const int left = Scale(12);
    const int right = rc.right - Scale(12);
    if (x < left) x = left;
    if (x > right) x = right;
    return 1 + MulDiv(x - left, MAX_CPS - 1, right - left);
}

void PaintSlider(HWND hwnd) {
    PAINTSTRUCT ps{};
    HDC dc = BeginPaint(hwnd, &ps);
    RECT rc{};
    GetClientRect(hwnd, &rc);
    FillRect(dc, &rc, g_bgBrush);

    const int left = Scale(12);
    const int right = rc.right - Scale(12);
    const int centerY = rc.bottom / 2;
    const int railH = Scale(6);
    const int thumb = Scale(20);
    const int thumbX = SliderXFromValue(hwnd);

    RECT rail{left, centerY - railH / 2, right, centerY + railH / 2};
    RECT fill{left, centerY - railH / 2, thumbX, centerY + railH / 2};

    HBRUSH railBrush = CreateSolidBrush(SliderTrackColor());
    HBRUSH fillBrush = CreateSolidBrush(AccentColor());
    HBRUSH thumbBrush = CreateSolidBrush(SliderThumbColor());
    HPEN noPen = CreatePen(PS_NULL, 0, 0);
    HPEN thumbPen = CreatePen(PS_SOLID, Scale(1), g_darkMode ? RGB(95, 95, 105) : RGB(190, 190, 196));

    HGDIOBJ oldBrush = SelectObject(dc, railBrush);
    HGDIOBJ oldPen = SelectObject(dc, noPen);
    RoundRect(dc, rail.left, rail.top, rail.right, rail.bottom, railH, railH);
    SelectObject(dc, fillBrush);
    RoundRect(dc, fill.left, fill.top, fill.right, fill.bottom, railH, railH);

    SelectObject(dc, thumbBrush);
    SelectObject(dc, thumbPen);
    Ellipse(dc, thumbX - thumb / 2, centerY - thumb / 2, thumbX + thumb / 2, centerY + thumb / 2);

    if (GetFocus() == hwnd) {
        RECT focus{thumbX - thumb / 2 - Scale(3), centerY - thumb / 2 - Scale(3), thumbX + thumb / 2 + Scale(3), centerY + thumb / 2 + Scale(3)};
        DrawFocusRect(dc, &focus);
    }

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(thumbPen);
    DeleteObject(noPen);
    DeleteObject(thumbBrush);
    DeleteObject(fillBrush);
    DeleteObject(railBrush);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK SliderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT:
        PaintSlider(hwnd);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_LBUTTONDOWN:
        SetFocus(hwnd);
        SetCapture(hwnd);
        g_draggingSlider = true;
        SetCpsValue(SliderValueFromX(hwnd, GET_X_LPARAM(lParam)));
        return 0;
    case WM_MOUSEMOVE:
        if (g_draggingSlider) {
            SetCpsValue(SliderValueFromX(hwnd, GET_X_LPARAM(lParam)));
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (g_draggingSlider) {
            g_draggingSlider = false;
            ReleaseCapture();
            SetCpsValue(SliderValueFromX(hwnd, GET_X_LPARAM(lParam)));
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wParam == VK_LEFT || wParam == VK_DOWN) {
            SetCpsValue(g_cps - 1);
            return 0;
        }
        if (wParam == VK_RIGHT || wParam == VK_UP) {
            SetCpsValue(g_cps + 1);
            return 0;
        }
        if (wParam == VK_PRIOR) {
            SetCpsValue(g_cps + 25);
            return 0;
        }
        if (wParam == VK_NEXT) {
            SetCpsValue(g_cps - 25);
            return 0;
        }
        if (wParam == VK_HOME) {
            SetCpsValue(1);
            return 0;
        }
        if (wParam == VK_END) {
            SetCpsValue(MAX_CPS);
            return 0;
        }
        break;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HFONT MakeFont(int points, int weight) {
    const UINT dpi = g_hwnd ? GetDpiForWindow(g_hwnd) : 96;
    LOGFONTW lf{};
    lf.lfHeight = -MulDiv(points, static_cast<int>(dpi), 72);
    lf.lfWeight = weight;
    lf.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(lf.lfFaceName, L"Segoe UI Variable Text");
    return CreateFontIndirectW(&lf);
}

void ApplyFonts() {
    if (g_titleFont) DeleteObject(g_titleFont);
    if (g_textFont) DeleteObject(g_textFont);
    g_titleFont = MakeFont(22, FW_SEMIBOLD);
    g_textFont = MakeFont(10, FW_NORMAL);

    HWND controls[] = {g_subtitle, g_cpsLabel, g_buttonLabel, g_cpsEdit, g_cpsSlider, g_leftRadio, g_rightRadio, g_startButton, g_exitButton, g_status, g_copyright};
    SendMessageW(g_title, WM_SETFONT, reinterpret_cast<WPARAM>(g_titleFont), TRUE);
    for (HWND control : controls) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_textFont), TRUE);
    }
}

void ApplyTheme() {
    if (g_bgBrush) DeleteObject(g_bgBrush);
    if (g_editBrush) DeleteObject(g_editBrush);
    g_bgBrush = CreateSolidBrush(BgColor());
    g_editBrush = CreateSolidBrush(EditBgColor());

    BOOL immersiveDark = g_darkMode ? TRUE : FALSE;
    DwmSetWindowAttribute(g_hwnd, 20, &immersiveDark, sizeof(immersiveDark));
    int cornerPreference = 2;
    DwmSetWindowAttribute(g_hwnd, 33, &cornerPreference, sizeof(cornerPreference));

    const wchar_t* theme = g_darkMode ? L"DarkMode_Explorer" : nullptr;
    HWND themedControls[] = {g_cpsEdit, g_startButton, g_exitButton};
    for (HWND control : themedControls) {
        SetWindowTheme(control, theme, nullptr);
        InvalidateRect(control, nullptr, TRUE);
    }

    InvalidateRect(g_leftRadio, nullptr, TRUE);
    InvalidateRect(g_rightRadio, nullptr, TRUE);
    InvalidateRect(g_hwnd, nullptr, TRUE);
}

void Layout() {
    RECT rc{};
    GetClientRect(g_hwnd, &rc);
    const int margin = Scale(28);
    const int gap = Scale(14);
    const int full = rc.right - (margin * 2);
    const int rowY = Scale(116);
    const int labelW = Scale(150);
    const int editW = Scale(120);
    const int buttonW = (full - gap) / 2;

    MoveWindow(g_title, margin, Scale(24), full, Scale(34), TRUE);
    MoveWindow(g_subtitle, margin, Scale(62), full, Scale(24), TRUE);
    MoveWindow(g_cpsLabel, margin, rowY, labelW, Scale(28), TRUE);
    MoveWindow(g_cpsEdit, rc.right - margin - editW, rowY - Scale(4), editW, Scale(32), TRUE);
    MoveWindow(g_cpsSlider, margin, Scale(152), full, Scale(36), TRUE);
    MoveWindow(g_buttonLabel, margin, Scale(214), labelW, Scale(28), TRUE);
    MoveWindow(g_leftRadio, rc.right - margin - Scale(252), Scale(212), Scale(116), Scale(30), TRUE);
    MoveWindow(g_rightRadio, rc.right - margin - Scale(126), Scale(212), Scale(126), Scale(30), TRUE);
    MoveWindow(g_startButton, margin, Scale(276), buttonW, Scale(42), TRUE);
    MoveWindow(g_exitButton, margin + buttonW + gap, Scale(276), buttonW, Scale(42), TRUE);
    MoveWindow(g_status, margin, rc.bottom - Scale(38), full - Scale(170), Scale(28), TRUE);
    MoveWindow(g_copyright, rc.right - margin - Scale(160), rc.bottom - Scale(38), Scale(160), Scale(28), TRUE);
}

void SetStatus(const wchar_t* text, StatusKind kind) {
    g_statusKind = kind;
    SetWindowTextW(g_status, text);
    if (kind == StatusKind::Running) SetTimer(g_hwnd, STATUS_TIMER, 520, nullptr);
    else KillTimer(g_hwnd, STATUS_TIMER);
    InvalidateRect(g_status, nullptr, TRUE);
}

void SendClick() {
    if (g_rightClick) {
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
    } else {
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    }
}

void SelectMouseButton(bool right) {
    g_rightClick = right;
    CheckDlgButton(g_hwnd, IDC_LEFT, right ? BST_UNCHECKED : BST_CHECKED);
    CheckDlgButton(g_hwnd, IDC_RIGHT, right ? BST_CHECKED : BST_UNCHECKED);
    InvalidateRect(g_leftRadio, nullptr, TRUE);
    InvalidateRect(g_rightRadio, nullptr, TRUE);
}

void DrawRadioButton(const DRAWITEMSTRUCT* item) {
    const bool checked = item->CtlID == IDC_RIGHT ? g_rightClick : !g_rightClick;
    const bool focused = (item->itemState & ODS_FOCUS) != 0;
    const int radio = Scale(16);
    RECT rc = item->rcItem;

    FillRect(item->hDC, &rc, g_bgBrush);
    SetBkMode(item->hDC, TRANSPARENT);
    SelectObject(item->hDC, g_textFont);

    const int cx = rc.left + radio / 2 + Scale(2);
    const int cy = rc.top + (rc.bottom - rc.top) / 2;
    RECT outer{cx - radio / 2, cy - radio / 2, cx + radio / 2, cy + radio / 2};

    HBRUSH outerBrush = CreateSolidBrush(BgColor());
    HPEN outerPen = CreatePen(PS_SOLID, Scale(1), checked ? AccentColor() : (g_darkMode ? RGB(145, 145, 150) : RGB(96, 96, 96)));
    HGDIOBJ oldBrush = SelectObject(item->hDC, outerBrush);
    HGDIOBJ oldPen = SelectObject(item->hDC, outerPen);
    Ellipse(item->hDC, outer.left, outer.top, outer.right, outer.bottom);
    SelectObject(item->hDC, oldPen);
    SelectObject(item->hDC, oldBrush);
    DeleteObject(outerPen);
    DeleteObject(outerBrush);

    if (checked) {
        const int dot = Scale(8);
        RECT inner{cx - dot / 2, cy - dot / 2, cx + dot / 2, cy + dot / 2};
        HBRUSH dotBrush = CreateSolidBrush(AccentColor());
        oldBrush = SelectObject(item->hDC, dotBrush);
        oldPen = SelectObject(item->hDC, GetStockObject(NULL_PEN));
        Ellipse(item->hDC, inner.left, inner.top, inner.right, inner.bottom);
        SelectObject(item->hDC, oldPen);
        SelectObject(item->hDC, oldBrush);
        DeleteObject(dotBrush);
    }

    wchar_t text[64]{};
    GetWindowTextW(item->hwndItem, text, 64);
    RECT textRc{rc.left + Scale(26), rc.top, rc.right, rc.bottom};
    SetTextColor(item->hDC, TextColor());
    DrawTextW(item->hDC, text, -1, &textRc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

    if (focused) {
        RECT focusRc = textRc;
        DrawFocusRect(item->hDC, &focusRc);
    }
}

DWORD WINAPI ClickThread(void*) {
    if (InterlockedCompareExchange(&g_skipCountdown, 0, 0) == 0) Sleep(1000);
    if (InterlockedCompareExchange(&g_stopRequested, 0, 0) != 0) return 0;

    PostMessageW(g_hwnd, WM_APP + 1, 0, 0);

    const LONGLONG rawInterval = g_freq.QuadPart / ClampCps(g_cps);
    const LONGLONG interval = rawInterval > 1 ? rawInterval : 1;
    LARGE_INTEGER next{};
    QueryPerformanceCounter(&next);
    next.QuadPart += interval;

    while (InterlockedCompareExchange(&g_stopRequested, 0, 0) == 0) {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        if (now.QuadPart >= next.QuadPart) {
            SendClick();
            next.QuadPart = now.QuadPart + interval;
            continue;
        }

        const LONGLONG remaining = next.QuadPart - now.QuadPart;
        if (remaining > (g_freq.QuadPart >> 9)) Sleep(1);
    }

    return 0;
}

void ReadSettings() {
    BOOL ok = FALSE;
    int cps = GetDlgItemInt(g_hwnd, IDC_CPS, &ok, FALSE);
    g_cps = ClampCps(ok ? cps : 1);
    g_syncingCps = true;
    SetDlgItemInt(g_hwnd, IDC_CPS, g_cps, FALSE);
    g_syncingCps = false;
    SetCpsValue(g_cps);
    g_rightClick = IsDlgButtonChecked(g_hwnd, IDC_RIGHT) == BST_CHECKED;
}

void SetCpsFromEdit() {
    if (g_syncingCps) return;

    BOOL ok = FALSE;
    int cps = GetDlgItemInt(g_hwnd, IDC_CPS, &ok, FALSE);
    if (!ok) return;
    const int clamped = ClampCps(cps);
    if (clamped != cps) {
        g_syncingCps = true;
        SetDlgItemInt(g_hwnd, IDC_CPS, clamped, FALSE);
        g_syncingCps = false;
    }
    SetCpsValue(clamped, false);
}

void StopClicking() {
    InterlockedExchange(&g_stopRequested, 1);
    InterlockedExchange(&g_running, 0);
    EnableWindow(g_startButton, TRUE);
    SetWindowTextW(g_startButton, L"Start");
    UnregisterHotKey(g_hwnd, HOTKEY_STOP);
    RegisterHotKey(g_hwnd, HOTKEY_START, MOD_SHIFT | MOD_NOREPEAT, VK_F6);
    SetStatus(L"Ready. Press Start to begin.", StatusKind::Ready);
}

void StartClicking(bool skipCountdown = false) {
    if (InterlockedCompareExchange(&g_running, 1, 0) != 0) return;

    ReadSettings();
    InterlockedExchange(&g_stopRequested, 0);
    InterlockedExchange(&g_skipCountdown, skipCountdown ? 1 : 0);
    EnableWindow(g_startButton, TRUE);
    SetWindowTextW(g_startButton, L"Stop");
    UnregisterHotKey(g_hwnd, HOTKEY_START);
    RegisterHotKey(g_hwnd, HOTKEY_STOP, MOD_NOREPEAT, VK_F6);
    SetStatus(skipCountdown ? L"Running. Press F6 to stop." : L"Starting in 1 second. Press F6 to stop.", StatusKind::Running);

    HANDLE thread = CreateThread(nullptr, 0, ClickThread, nullptr, 0, nullptr);
    if (!thread) {
        StopClicking();
        SetStatus(L"Could not start click thread.", StatusKind::Error);
        return;
    }
    CloseHandle(thread);
}

HWND CreateControl(const wchar_t* klass, const wchar_t* text, DWORD style, int id) {
    return CreateWindowExW(
        0, klass, text, WS_CHILD | WS_VISIBLE | style,
        0, 0, 10, 10, g_hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
}

void RegisterSliderClass() {
    WNDCLASSW wc{};
    wc.lpfnWndProc = SliderWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = CPS_SLIDER_CLASS;
    wc.hCursor = LoadCursorW(nullptr, IDC_HAND);
    wc.hbrBackground = nullptr;
    RegisterClassW(&wc);
}

void CreateUi(HWND hwnd) {
    g_hwnd = hwnd;

    g_title = CreateControl(L"STATIC", L"Autoclicker", SS_LEFT, IDC_TITLE);
    g_subtitle = CreateControl(L"STATIC", L"Keybinds: Shift+F6 start instantly, F6 stop", SS_LEFT, IDC_SUBTITLE);
    g_cpsLabel = CreateControl(L"STATIC", L"Clicks per second", SS_LEFT, IDC_CPS_LABEL);
    g_buttonLabel = CreateControl(L"STATIC", L"Mouse button", SS_LEFT, IDC_BUTTON_LABEL);
    g_cpsEdit = CreateControl(L"EDIT", L"12", WS_BORDER | WS_TABSTOP | ES_NUMBER, IDC_CPS);
    g_cpsSlider = CreateControl(CPS_SLIDER_CLASS, L"", WS_TABSTOP, IDC_CPS_SLIDER);
    g_leftRadio = CreateControl(L"BUTTON", L"Left click", BS_AUTORADIOBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_LEFT);
    g_rightRadio = CreateControl(L"BUTTON", L"Right click", BS_AUTORADIOBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_RIGHT);
    g_startButton = CreateControl(L"BUTTON", L"Start", BS_PUSHBUTTON | WS_TABSTOP, IDC_START);
    g_exitButton = CreateControl(L"BUTTON", L"Exit", BS_PUSHBUTTON | WS_TABSTOP, IDC_EXIT);
    g_status = CreateControl(L"STATIC", L"Ready. Press Start to begin.", SS_LEFT, IDC_STATUS);
    g_copyright = CreateControl(L"STATIC", L"\u00A9 2026 kantiDev", SS_RIGHT | SS_NOTIFY, IDC_COPYRIGHT);

    SetCpsValue(g_cps);
    SelectMouseButton(false);
    ApplyFonts();
    ApplyTheme();
    Layout();
    RegisterHotKey(g_hwnd, HOTKEY_START, MOD_SHIFT | MOD_NOREPEAT, VK_F6);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateUi(hwnd);
        return 0;
    case WM_SIZE:
        Layout();
        return 0;
    case WM_ERASEBKGND: {
        RECT rc{};
        GetClientRect(hwnd, &rc);
        FillRect(reinterpret_cast<HDC>(wParam), &rc, g_bgBrush);
        return 1;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, control == g_status ? StatusColor() : (control == g_subtitle || control == g_copyright ? MutedTextColor() : TextColor()));
        return reinterpret_cast<LRESULT>(g_bgBrush);
    }
    case WM_CTLCOLORBTN: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, TextColor());
        return reinterpret_cast<LRESULT>(g_bgBrush);
    }
    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetBkColor(dc, EditBgColor());
        SetTextColor(dc, TextColor());
        return reinterpret_cast<LRESULT>(g_editBrush);
    }
    case WM_DPICHANGED: {
        RECT* suggested = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        ApplyFonts();
        Layout();
        return 0;
    }
    case WM_TIMER:
        if (wParam == STATUS_TIMER) {
            g_statusPulse = !g_statusPulse;
            InvalidateRect(g_status, nullptr, TRUE);
            return 0;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_CPS:
            if (HIWORD(wParam) == EN_CHANGE && GetFocus() == g_cpsEdit) SetCpsFromEdit();
            return 0;
        case IDC_LEFT:
            SelectMouseButton(false);
            return 0;
        case IDC_RIGHT:
            SelectMouseButton(true);
            return 0;
        case IDC_START:
            if (InterlockedCompareExchange(&g_running, 0, 0) != 0) StopClicking();
            else StartClicking();
            return 0;
        case IDC_EXIT:
            DestroyWindow(hwnd);
            return 0;
        case IDC_COPYRIGHT:
            if (HIWORD(wParam) == STN_CLICKED) {
                if (GetKeyState(VK_CONTROL) & 0x8000) {
                    ShellExecuteW(hwnd, L"open", L"https://github.com/TheOriUHD/", nullptr, nullptr, SW_SHOWNORMAL);
                } else {
                    g_darkMode = !g_darkMode;
                    ApplyTheme();
                }
            }
            return 0;
        }
        break;
    case WM_DRAWITEM: {
        const auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (item->CtlID == IDC_LEFT || item->CtlID == IDC_RIGHT) {
            DrawRadioButton(item);
            return TRUE;
        }
        break;
    }
    case WM_HOTKEY:
        if (wParam == HOTKEY_START) StartClicking(true);
        if (wParam == HOTKEY_STOP) StopClicking();
        return 0;
    case WM_APP + 1:
        SetStatus(L"Running. Press F6 to stop.", StatusKind::Running);
        return 0;
    case WM_DESTROY:
        UnregisterHotKey(g_hwnd, HOTKEY_START);
        StopClicking();
        DeleteObject(g_titleFont);
        DeleteObject(g_textFont);
        DeleteObject(g_bgBrush);
        DeleteObject(g_editBrush);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    QueryPerformanceFrequency(&g_freq);

    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);
    RegisterSliderClass();

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"ModernAsmAutoclickerWindow";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    const int width = MulDiv(520, GetDpiForSystem(), 96);
    const int height = MulDiv(460, GetDpiForSystem(), 96);
    HWND hwnd = CreateWindowExW(
        0, wc.lpszClassName, L"Autoclicker",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 1;

    HICON icon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
