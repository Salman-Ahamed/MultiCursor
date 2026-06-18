#include "WindowManager.h"
#include "Core/Logger.h"
#include <dwmapi.h>
#include <wtsapi32.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "wtsapi32.lib")

WindowManager* WindowManager::s_instance = nullptr;

WindowManager::WindowManager(AppStateMachine& stateMachine)
    : m_stateMachine(stateMachine) {
    s_instance = this;
}

WindowManager::~WindowManager() {
    Shutdown();
    if (s_instance == this) s_instance = nullptr;
}

bool WindowManager::Initialize() {
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"MultiCursorOverlay";

    if (!RegisterClassExW(&wc)) {
        LOG_ERROR(L"Failed to register overlay window class: %d", GetLastError());
        return false;
    }

    if (!CreateOverlayWindow()) return false;

    WTSRegisterSessionNotification(m_hwnd, NOTIFY_FOR_THIS_SESSION);

    LOG_INFO(L"WindowManager initialized");
    return true;
}

bool WindowManager::CreateOverlayWindow() {
    int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    m_hwnd = CreateWindowExW(
        WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        L"MultiCursorOverlay", L"MultiCursor",
        WS_POPUP,
        vx, vy, vw, vh,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);

    if (!m_hwnd) {
        LOG_ERROR(L"Failed to create overlay window: %d", GetLastError());
        return false;
    }

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    BOOL dark = TRUE;
    DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    return true;
}

void WindowManager::Shutdown() {
    if (m_hwnd) {
        WTSUnRegisterSessionNotification(m_hwnd);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void WindowManager::Show() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void WindowManager::Hide() {
    if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
}

void WindowManager::SetOverlayEnabled(bool enabled) {
    m_overlayEnabled = enabled;
    if (enabled) Show();
    else Hide();
}

void WindowManager::HandleDpiChange(WPARAM wParam, LPARAM lParam) {
    RECT* rect = (RECT*)lParam;
    if (rect) {
        SetWindowPos(m_hwnd, HWND_TOPMOST,
                     rect->left, rect->top,
                     rect->right - rect->left,
                     rect->bottom - rect->top,
                     SWP_NOACTIVATE | SWP_NOZORDER);
    }
}

void WindowManager::HandleSessionChange(WPARAM wParam) {
    if (wParam == WTS_SESSION_LOCK) {
        m_stateMachine.TransitionTo(AppState::Suspended);
    } else if (wParam == WTS_SESSION_UNLOCK) {
        m_stateMachine.TransitionTo(AppState::Running);
    }
}

void WindowManager::HandleDisplayChange() {
    int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    LOG_INFO(L"Display changed: %dx%d (%d,%d)", vw, vh, vx, vy);

    if (m_hwnd) {
        SetWindowPos(m_hwnd, HWND_TOPMOST, vx, vy, vw, vh,
                     SWP_NOACTIVATE | SWP_NOZORDER);
    }

    if (m_displayChangeCb) m_displayChangeCb(vw, vh);
}

void WindowManager::HandlePowerBroadcast(WPARAM wParam) {
    if (wParam == PBT_APMRESUMEAUTOMATIC) {
        LOG_INFO(L"System resumed from sleep");
        if (m_overlayEnabled) Show();
    }
}

LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = (WindowManager*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_DPICHANGED:
        if (self) self->HandleDpiChange(wParam, lParam);
        return 0;

    case WM_WTSSESSION_CHANGE:
        if (self) self->HandleSessionChange(wParam);
        return 0;

    case WM_DISPLAYCHANGE:
        if (self) self->HandleDisplayChange();
        return 0;

    case WM_POWERBROADCAST:
        if (self) self->HandlePowerBroadcast(wParam);
        return TRUE;

    case WM_QUERYENDSESSION:
        return TRUE;

    case WM_ENDSESSION:
        if (wParam) PostQuitMessage(0);
        return 0;

    case WM_NCHITTEST:
        return HTTRANSPARENT;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
