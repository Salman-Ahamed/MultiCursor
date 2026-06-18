#include "SettingsWindow.h"
#include "Core/Logger.h"
#include <windowsx.h>
#include <commctrl.h>

SettingsWindow* SettingsWindow::s_instance = nullptr;

SettingsWindow::SettingsWindow() {
    s_instance = this;
}

SettingsWindow::~SettingsWindow() {
    if (m_hwnd) DestroyWindow(m_hwnd);
    if (s_instance == this) s_instance = nullptr;
}

bool SettingsWindow::Initialize(HINSTANCE hInstance, HWND parent, SettingsManager& settings) {
    m_parent = parent;
    m_settings = &settings;

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
        LoadSettings();
        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
    }
}

void SettingsWindow::OnCreate() {
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(m_hwnd, GWLP_HINSTANCE);

    CreateWindowExW(0, L"STATIC", L"Overlay Enabled:",
                    WS_CHILD | WS_VISIBLE,
                    20, 20, 120, 20, m_hwnd, nullptr, hInst, nullptr);

    m_overlayCheck = CreateWindowExW(0, L"BUTTON", L"",
                                      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                      150, 20, 20, 20, m_hwnd, (HMENU)2001, hInst, nullptr);

    CreateWindowExW(0, L"STATIC", L"Show Labels:",
                    WS_CHILD | WS_VISIBLE,
                    20, 50, 120, 20, m_hwnd, nullptr, hInst, nullptr);

    m_labelsCheck = CreateWindowExW(0, L"BUTTON", L"",
                                     WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                     150, 50, 20, 20, m_hwnd, (HMENU)2002, hInst, nullptr);

    CreateWindowExW(0, L"STATIC", L"Cursor Size:",
                    WS_CHILD | WS_VISIBLE,
                    20, 80, 120, 20, m_hwnd, nullptr, hInst, nullptr);

    m_cursorSizeSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                          WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                                          150, 80, 150, 25, m_hwnd, (HMENU)2003, hInst, nullptr);

    m_cursorSizeLabel = CreateWindowExW(0, L"STATIC", L"12",
                                         WS_CHILD | WS_VISIBLE,
                                         310, 80, 40, 20, m_hwnd, nullptr, hInst, nullptr);

    if (m_cursorSizeSlider) {
        SendMessageW(m_cursorSizeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(4, 48));
        SendMessageW(m_cursorSizeSlider, TBM_SETPAGESIZE, 0, 4);
    }

    CreateWindowExW(0, L"STATIC", L"Opacity:",
                    WS_CHILD | WS_VISIBLE,
                    20, 110, 120, 20, m_hwnd, nullptr, hInst, nullptr);

    m_opacitySlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                       WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                                       150, 110, 150, 25, m_hwnd, (HMENU)2004, hInst, nullptr);

    if (m_opacitySlider) {
        SendMessageW(m_opacitySlider, TBM_SETRANGE, TRUE, MAKELPARAM(10, 100));
        SendMessageW(m_opacitySlider, TBM_SETPAGESIZE, 0, 10);
    }

    CreateWindowExW(0, L"BUTTON", L"Save",
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    150, 160, 80, 30, m_hwnd, (HMENU)2005, hInst, nullptr);
}

void SettingsWindow::LoadSettings() {
    if (!m_settings) return;

    if (m_overlayCheck)
        Button_SetCheck(m_overlayCheck, m_settings->GetBool("overlay_enabled", true) ? BST_CHECKED : BST_UNCHECKED);

    if (m_labelsCheck)
        Button_SetCheck(m_labelsCheck, m_settings->GetBool("show_labels", true) ? BST_CHECKED : BST_UNCHECKED);

    if (m_cursorSizeSlider) {
        int size = m_settings->GetInt("cursor_size", 12);
        SendMessageW(m_cursorSizeSlider, TBM_SETPOS, TRUE, size);
        wchar_t buf[16]; swprintf_s(buf, L"%d", size);
        SetWindowTextW(m_cursorSizeLabel, buf);
    }

    if (m_opacitySlider) {
        int opacity = (int)(m_settings->GetFloat("cursor_opacity", 0.8f) * 100.0f);
        SendMessageW(m_opacitySlider, TBM_SETPOS, TRUE, opacity);
    }
}

void SettingsWindow::SaveSettings() {
    if (!m_settings) return;

    if (m_overlayCheck)
        m_settings->SetBool("overlay_enabled", Button_GetCheck(m_overlayCheck) == BST_CHECKED);

    if (m_labelsCheck)
        m_settings->SetBool("show_labels", Button_GetCheck(m_labelsCheck) == BST_CHECKED);

    if (m_cursorSizeSlider) {
        LRESULT pos = SendMessageW(m_cursorSizeSlider, TBM_GETPOS, 0, 0);
        m_settings->SetInt("cursor_size", (int)pos);
    }

    if (m_opacitySlider) {
        LRESULT pos = SendMessageW(m_opacitySlider, TBM_GETPOS, 0, 0);
        m_settings->SetFloat("cursor_opacity", (float)pos / 100.0f);
    }

    m_settings->Save();
    LOG_INFO(L"Settings saved");
}

LRESULT CALLBACK SettingsWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = (SettingsWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE:
        if (self) self->OnCreate();
        return 0;

    case WM_HSCROLL:
        if (self && self->m_cursorSizeSlider && (HWND)lParam == self->m_cursorSizeSlider) {
            LRESULT pos = SendMessageW(self->m_cursorSizeSlider, TBM_GETPOS, 0, 0);
            wchar_t buf[16]; swprintf_s(buf, L"%d", (int)pos);
            SetWindowTextW(self->m_cursorSizeLabel, buf);
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == 2005) {
            if (self) self->SaveSettings();
        }
        return 0;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
