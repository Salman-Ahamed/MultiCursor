#include "MainWindow.h"
#include "SettingsWindow.h"
#include "Core/Logger.h"
#include <commctrl.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

MainWindow* MainWindow::s_instance = nullptr;

MainWindow::MainWindow() {
    s_instance = this;
    m_darkBgBrush = CreateSolidBrush(RGB(32, 32, 32));
}

MainWindow::~MainWindow() {
    if (m_darkBgBrush) DeleteObject(m_darkBgBrush);
    if (s_instance == this) s_instance = nullptr;
}

bool MainWindow::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(101));
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MultiCursorMainWindow";

    if (!RegisterClassExW(&wc)) {
        LOG_ERROR(L"Failed to register MainWindow class: %d", GetLastError());
        return false;
    }

    m_hwnd = CreateWindowExW(0, L"MultiCursorMainWindow", L"MultiCursor - Device Manager",
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
                              nullptr, nullptr, hInstance, this);

    if (!m_hwnd) {
        LOG_ERROR(L"Failed to create MainWindow: %d", GetLastError());
        return false;
    }
    // GWLP_USERDATA set in WM_NCCREATE via CREATESTRUCT.lpCreateParams

    BOOL dark = TRUE;
    DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    LOG_INFO(L"MainWindow initialized");
    return true;
}

void MainWindow::Show(int nCmdShow) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, nCmdShow);
        SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        UpdateWindow(m_hwnd);
    }
}

void MainWindow::OnCreate() {
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    m_deviceList = CreateWindowExW(0, WC_LISTVIEWW, L"",
                                    WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                    10, 10, rc.right - 20, rc.bottom - 80,
                                    m_hwnd, nullptr, m_hInstance, nullptr);

    if (m_deviceList) {
        LVCOLUMNW col = {};
        col.mask = LVCF_TEXT | LVCF_WIDTH;

        col.pszText = (LPWSTR)L"Device";
        col.cx = 200;
        ListView_InsertColumn(m_deviceList, 0, &col);

        col.pszText = (LPWSTR)L"Status";
        col.cx = 80;
        ListView_InsertColumn(m_deviceList, 1, &col);

        col.pszText = (LPWSTR)L"Color";
        col.cx = 50;
        ListView_InsertColumn(m_deviceList, 2, &col);

        col.pszText = (LPWSTR)L"Cursor";
        col.cx = 50;
        ListView_InsertColumn(m_deviceList, 3, &col);
    }

    HWND hSettings = CreateWindowExW(0, L"BUTTON", L"Settings",
                                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      10, rc.bottom - 60, 100, 30,
                                      m_hwnd, (HMENU)1001, m_hInstance, nullptr);

    HWND hQuit = CreateWindowExW(0, L"BUTTON", L"Quit",
                                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                  rc.right - 90, rc.bottom - 60, 80, 30,
                                  m_hwnd, (HMENU)1002, m_hInstance, nullptr);

    m_statusBar = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr,
                                   WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                   0, 0, 0, 0, m_hwnd, (HMENU)1003, m_hInstance, nullptr);
}

void MainWindow::OnDeviceListUpdate() {
    RefreshDeviceList();
}

void MainWindow::RefreshDeviceList() {
    if (!m_deviceList || !m_deviceMgr) return;

    ListView_DeleteAllItems(m_deviceList);

    const auto& devices = m_deviceMgr->Devices();
    for (size_t i = 0; i < devices.size(); i++) {
        auto& dev = devices[i];
        if (!dev.isMouse) continue;

        wchar_t label[256];
        auto hashPos = dev.name.find_last_of(L'#');
        if (hashPos != std::wstring::npos) {
            auto colPos = dev.name.rfind(L'#', hashPos - 1);
            if (colPos != std::wstring::npos && colPos + 1 < dev.name.size()) {
                wcsncpy_s(label, dev.name.c_str() + colPos + 1, hashPos - colPos - 1);
                label[hashPos - colPos - 1] = 0;
            } else {
                wcsncpy_s(label, dev.name.c_str(), hashPos);
                label[hashPos] = 0;
            }
        } else {
            wcsncpy_s(label, dev.name.c_str(), _TRUNCATE);
        }

        LVITEMW item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = (int)i;
        item.pszText = label;
        item.lParam = (LPARAM)dev.hDevice;
        int idx = ListView_InsertItem(m_deviceList, &item);

        if (idx >= 0) {
            ListView_SetItemText(m_deviceList, idx, 1, (LPWSTR)L"Active");
            DWORD color = 0xFFE6194B;
            if (m_cursorMgr) {
                UINT cIdx = m_cursorMgr->LookupCursor(dev.hDevice);
                if (cIdx != (UINT)-1) {
                    auto* cs = m_cursorMgr->GetCursor(cIdx);
                    if (cs) color = cs->color;
                }
            }
            wchar_t colorStr[16]; swprintf_s(colorStr, L"0x%08X", color);
            ListView_SetItemText(m_deviceList, idx, 2, colorStr);
            wchar_t buf[16]; swprintf_s(buf, L"%zu", i + 1);
            ListView_SetItemText(m_deviceList, idx, 3, buf);
        }
    }
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    auto* self = (MainWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE:
        if (self) self->OnCreate();
        return 0;

    case WM_SIZE:
        if (self && self->m_deviceList) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            SetWindowPos(self->m_deviceList, nullptr, 10, 10,
                         rc.right - 20, rc.bottom - 80, SWP_NOZORDER);
            if (self->m_statusBar) {
                SendMessageW(self->m_statusBar, WM_SIZE, 0, 0);
            }
        }
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

    case WM_COMMAND:
        if (LOWORD(wParam) == 1001) {
            LOG_INFO(L"Settings button clicked");
            if (self && self->m_settingsWindow) {
                self->m_settingsWindow->Show();
            }
        } else if (LOWORD(wParam) == 1002) {
            PostQuitMessage(0);
        }
        return 0;

    case WM_NOTIFY: {
        LPNMHDR nmh = (LPNMHDR)lParam;
        if (nmh->hwndFrom == self->m_deviceList && nmh->code == NM_CUSTOMDRAW) {
            LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)lParam;
            if (cd->nmcd.dwDrawStage == CDDS_PREPAINT)
                return CDRF_NOTIFYITEMDRAW;
            if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
                return CDRF_NOTIFYSUBITEMDRAW;
            if (cd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM) && cd->iSubItem == 2) {
                LVITEMW lvi = {};
                lvi.mask = LVIF_PARAM;
                lvi.iItem = (int)cd->nmcd.dwItemSpec;
                ListView_GetItem(self->m_deviceList, &lvi);
                HANDLE hDev = (HANDLE)lvi.lParam;
                DWORD color = 0xFFE6194B;
                if (self->m_cursorMgr) {
                    UINT cIdx = self->m_cursorMgr->LookupCursor(hDev);
                    if (cIdx != (UINT)-1) {
                        auto* cs = self->m_cursorMgr->GetCursor(cIdx);
                        if (cs) color = cs->color;
                    }
                }
                RECT rc;
                ListView_GetSubItemRect(self->m_deviceList, (int)cd->nmcd.dwItemSpec, 2, LVIR_BOUNDS, &rc);
                InflateRect(&rc, -3, -3);
                HBRUSH brush = CreateSolidBrush(RGB(GetRValue(color), GetGValue(color), GetBValue(color)));
                if (brush) {
                    FillRect(cd->nmcd.hdc, &rc, brush);
                    DeleteObject(brush);
                }
                FrameRect(cd->nmcd.hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
                SetBkMode(cd->nmcd.hdc, TRANSPARENT);
                return CDRF_SKIPDEFAULT;
            }
        }
        break;
    }

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_APP + 2:
        if (self) self->RefreshDeviceList();
        return 0;

    case WM_APP + 1:
        if (!self) break;
        switch (lParam) {
        case WM_LBUTTONDBLCLK:
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            break;
        case WM_RBUTTONUP: {
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 3001, L"Show/Hide");
            AppendMenuW(hMenu, MF_STRING, 3002, L"Settings");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, 3003, L"Exit");
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(hMenu);
            if (cmd == 3001) {
                if (IsWindowVisible(hwnd))
                    ShowWindow(hwnd, SW_HIDE);
                else
                    ShowWindow(hwnd, SW_SHOW);
            } else if (cmd == 3002) {
                ShowWindow(hwnd, SW_SHOW);
                if (self->m_settingsWindow) self->m_settingsWindow->Show();
            } else if (cmd == 3003) {
                PostQuitMessage(0);
            }
            break;
        }
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
