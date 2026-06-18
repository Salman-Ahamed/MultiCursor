#include "SettingsWindow.h"
#include "Core/Logger.h"
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

SettingsWindow* SettingsWindow::s_instance = nullptr;

SettingsWindow::SettingsWindow() {
    s_instance = this;
    m_darkBgBrush = CreateSolidBrush(RGB(32, 32, 32));
}

SettingsWindow::~SettingsWindow() {
    if (m_darkBgBrush) DeleteObject(m_darkBgBrush);
    if (m_hwnd) DestroyWindow(m_hwnd);
    if (s_instance == this) s_instance = nullptr;
}

bool SettingsWindow::Initialize(HINSTANCE hInstance, HWND parent, SettingsManager& settings) {
    m_parent = parent;
    m_settings = &settings;
    m_cursorMgr = nullptr;

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
                              CW_USEDEFAULT, CW_USEDEFAULT, 420, 400,
                              parent, nullptr, hInstance, this);

    if (!m_hwnd) {
        LOG_ERROR(L"Failed to create SettingsWindow: %d", GetLastError());
        return false;
    }
    // GWLP_USERDATA set in WM_NCCREATE via CREATESTRUCT.lpCreateParams

    BOOL dark = TRUE;
    DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

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
    int y = 20;

    CreateWindowExW(0, L"STATIC", L"Device:",
                    WS_CHILD | WS_VISIBLE,
                    20, y, 120, 20, m_hwnd, nullptr, hInst, nullptr);

    m_deviceCombo = CreateWindowExW(0, WC_COMBOBOXW, L"",
                                     WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                     150, y, 200, 120, m_hwnd, (HMENU)2010, hInst, nullptr);
    y += 30;

    CreateWindowExW(0, L"STATIC", L"Overlay Enabled:",
                    WS_CHILD | WS_VISIBLE,
                    20, y, 120, 20, m_hwnd, nullptr, hInst, nullptr);

    m_overlayCheck = CreateWindowExW(0, L"BUTTON", L"",
                                      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                      150, y, 20, 20, m_hwnd, (HMENU)2001, hInst, nullptr);
    y += 30;

    CreateWindowExW(0, L"STATIC", L"Show Labels:",
                    WS_CHILD | WS_VISIBLE,
                    20, y, 120, 20, m_hwnd, nullptr, hInst, nullptr);

    m_labelsCheck = CreateWindowExW(0, L"BUTTON", L"",
                                     WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                     150, y, 20, 20, m_hwnd, (HMENU)2002, hInst, nullptr);
    y += 30;

    CreateWindowExW(0, L"STATIC", L"Log Level:",
                    WS_CHILD | WS_VISIBLE,
                    20, y, 120, 20, m_hwnd, nullptr, hInst, nullptr);

    m_logLevelCombo = CreateWindowExW(0, WC_COMBOBOXW, L"",
                                       WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                       150, y, 100, 80, m_hwnd, (HMENU)2011, hInst, nullptr);
    if (m_logLevelCombo) {
        SendMessageW(m_logLevelCombo, CB_ADDSTRING, 0, (LPARAM)L"Debug");
        SendMessageW(m_logLevelCombo, CB_ADDSTRING, 0, (LPARAM)L"Info");
        SendMessageW(m_logLevelCombo, CB_ADDSTRING, 0, (LPARAM)L"Warn");
        SendMessageW(m_logLevelCombo, CB_ADDSTRING, 0, (LPARAM)L"Error");
        SendMessageW(m_logLevelCombo, CB_SETCURSEL, 1, 0);
    }
    y += 30;

    CreateWindowExW(0, L"STATIC", L"Cursor Size:",
                    WS_CHILD | WS_VISIBLE,
                    20, y, 120, 20, m_hwnd, nullptr, hInst, nullptr);

    m_cursorSizeSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                          WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                                          150, y, 150, 25, m_hwnd, (HMENU)2003, hInst, nullptr);

    m_cursorSizeLabel = CreateWindowExW(0, L"STATIC", L"12",
                                         WS_CHILD | WS_VISIBLE,
                                         310, y, 40, 20, m_hwnd, nullptr, hInst, nullptr);

    if (m_cursorSizeSlider) {
        SendMessageW(m_cursorSizeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(4, 48));
        SendMessageW(m_cursorSizeSlider, TBM_SETPAGESIZE, 0, 4);
    }
    y += 30;

    CreateWindowExW(0, L"STATIC", L"Opacity:",
                    WS_CHILD | WS_VISIBLE,
                    20, y, 120, 20, m_hwnd, nullptr, hInst, nullptr);

    m_opacitySlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                       WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                                       150, y, 150, 25, m_hwnd, (HMENU)2004, hInst, nullptr);

    if (m_opacitySlider) {
        SendMessageW(m_opacitySlider, TBM_SETRANGE, TRUE, MAKELPARAM(10, 100));
        SendMessageW(m_opacitySlider, TBM_SETPAGESIZE, 0, 10);
    }
    y += 30;

    CreateWindowExW(0, L"STATIC", L"Speed:",
                    WS_CHILD | WS_VISIBLE,
                    20, y, 120, 20, m_hwnd, nullptr, hInst, nullptr);

    m_speedSlider = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                                     WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                                     150, y, 150, 25, m_hwnd, (HMENU)2012, hInst, nullptr);

    m_speedLabel = CreateWindowExW(0, L"STATIC", L"1.0x",
                                    WS_CHILD | WS_VISIBLE,
                                    310, y, 40, 20, m_hwnd, nullptr, hInst, nullptr);

    if (m_speedSlider) {
        SendMessageW(m_speedSlider, TBM_SETRANGE, TRUE, MAKELPARAM(10, 50));
        SendMessageW(m_speedSlider, TBM_SETPAGESIZE, 0, 5);
        SendMessageW(m_speedSlider, TBM_SETPOS, TRUE, 10);
    }
    y += 30;

    CreateWindowExW(0, L"BUTTON", L"Save",
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    150, y, 80, 30, m_hwnd, (HMENU)2005, hInst, nullptr);
}

void SettingsWindow::LoadSettings() {
    if (!m_settings) return;

    if (m_overlayCheck)
        Button_SetCheck(m_overlayCheck, m_settings->GetBool("overlay_enabled", true) ? BST_CHECKED : BST_UNCHECKED);

    if (m_labelsCheck)
        Button_SetCheck(m_labelsCheck, m_settings->GetBool("show_labels", true) ? BST_CHECKED : BST_UNCHECKED);

    if (m_logLevelCombo) {
        int level = m_settings->GetInt("log_level_int", 1);
        SendMessageW(m_logLevelCombo, CB_SETCURSEL, (WPARAM)level, 0);
    }

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

    if (m_speedSlider) {
        float speed = m_settings->GetFloat("cursor_speed", 1.0f);
        int pos = (int)(speed * 10.0f);
        SendMessageW(m_speedSlider, TBM_SETPOS, TRUE, pos);
        wchar_t buf[16]; swprintf_s(buf, L"%.1fx", speed);
        SetWindowTextW(m_speedLabel, buf);
    }

    if (m_deviceCombo && m_cursorMgr) {
        SendMessageW(m_deviceCombo, CB_RESETCONTENT, 0, 0);
        std::vector<CursorState> cursors;
        m_cursorMgr->GetAllCursors(cursors);
        for (auto& c : cursors) {
            wchar_t buf[128];
            swprintf_s(buf, L"Cursor %u - %s", c.id, c.label.c_str());
            SendMessageW(m_deviceCombo, CB_ADDSTRING, 0, (LPARAM)buf);
        }
        SendMessageW(m_deviceCombo, CB_SETCURSEL, 0, 0);
    }
}

void SettingsWindow::SaveSettings() {
    if (!m_settings) return;

    if (m_overlayCheck)
        m_settings->SetBool("overlay_enabled", Button_GetCheck(m_overlayCheck) == BST_CHECKED);

    if (m_labelsCheck)
        m_settings->SetBool("show_labels", Button_GetCheck(m_labelsCheck) == BST_CHECKED);

    if (m_logLevelCombo) {
        LRESULT sel = SendMessageW(m_logLevelCombo, CB_GETCURSEL, 0, 0);
        if (sel != CB_ERR) {
            m_settings->SetInt("log_level_int", (int)sel);
            Logger::Instance().SetLevel((Severity)sel);
        }
    }

    if (m_cursorSizeSlider) {
        LRESULT pos = SendMessageW(m_cursorSizeSlider, TBM_GETPOS, 0, 0);
        m_settings->SetInt("cursor_size", (int)pos);
    }

    if (m_opacitySlider) {
        LRESULT pos = SendMessageW(m_opacitySlider, TBM_GETPOS, 0, 0);
        m_settings->SetFloat("cursor_opacity", (float)pos / 100.0f);
    }

    if (m_speedSlider) {
        LRESULT pos = SendMessageW(m_speedSlider, TBM_GETPOS, 0, 0);
        m_settings->SetFloat("cursor_speed", (float)pos / 10.0f);
    }

    m_settings->Save();
    LOG_INFO(L"Settings saved");
}

LRESULT CALLBACK SettingsWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    auto* self = (SettingsWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE:
        if (self) self->OnCreate();
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        if (self) {
            SetBkColor((HDC)wParam, RGB(32, 32, 32));
            SetTextColor((HDC)wParam, RGB(200, 200, 200));
            return (LRESULT)self->m_darkBgBrush;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_ERASEBKGND:
        if (self) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wParam, &rc, self->m_darkBgBrush);
            return 1;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);

    case WM_HSCROLL:
        if (self) {
            if ((HWND)lParam == self->m_cursorSizeSlider) {
                LRESULT pos = SendMessageW(self->m_cursorSizeSlider, TBM_GETPOS, 0, 0);
                wchar_t buf[16]; swprintf_s(buf, L"%d", (int)pos);
                SetWindowTextW(self->m_cursorSizeLabel, buf);
            } else if ((HWND)lParam == self->m_speedSlider) {
                LRESULT pos = SendMessageW(self->m_speedSlider, TBM_GETPOS, 0, 0);
                wchar_t buf[16]; swprintf_s(buf, L"%.1fx", (float)pos / 10.0f);
                SetWindowTextW(self->m_speedLabel, buf);
            }
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
