#include "SettingsWindow.h"
#include "Core/Logger.h"

SettingsWindow* SettingsWindow::s_instance = nullptr;

SettingsWindow::SettingsWindow() {
    s_instance = this;
}

SettingsWindow::~SettingsWindow() {
    if (s_instance == this) s_instance = nullptr;
}

bool SettingsWindow::Initialize(HINSTANCE hInstance, HWND parent) {
    m_parent = parent;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MultiCursorSettingsWindow";

    if (!RegisterClassExW(&wc)) {
        LOG_ERROR(L"Failed to register SettingsWindow class: %d", GetLastError());
        return false;
    }

    m_hwnd = CreateWindowExW(0, L"MultiCursorSettingsWindow", L"MultiCursor Settings",
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
                              parent, nullptr, hInstance, nullptr);

    if (!m_hwnd) {
        LOG_ERROR(L"Failed to create SettingsWindow: %d", GetLastError());
        return false;
    }

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    LOG_INFO(L"SettingsWindow initialized");
    return true;
}

void SettingsWindow::Show() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
    }
}

void SettingsWindow::OnCreate() {
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    CreateWindowExW(0, L"STATIC", L"Cursor Size:",
                    WS_CHILD | WS_VISIBLE,
                    20, 20, 120, 20, m_hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);

    CreateWindowExW(0, L"STATIC", L"Overlay Enabled:",
                    WS_CHILD | WS_VISIBLE,
                    20, 50, 120, 20, m_hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);
}

LRESULT CALLBACK SettingsWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = (SettingsWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE:
        if (self) self->OnCreate();
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
