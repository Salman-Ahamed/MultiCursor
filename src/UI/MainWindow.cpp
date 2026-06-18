#include "MainWindow.h"
#include "SettingsWindow.h"
#include "Core/Logger.h"
#include <commctrl.h>

MainWindow* MainWindow::s_instance = nullptr;

MainWindow::MainWindow() {
    s_instance = this;
}

MainWindow::~MainWindow() {
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

        col.pszText = (LPWSTR)L"Cursor";
        col.cx = 60;
        ListView_InsertColumn(m_deviceList, 2, &col);
    }

    HWND hSettings = CreateWindowExW(0, L"BUTTON", L"Settings",
                                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      10, rc.bottom - 60, 100, 30,
                                      m_hwnd, (HMENU)1001, m_hInstance, nullptr);

    HWND hQuit = CreateWindowExW(0, L"BUTTON", L"Quit",
                                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                  rc.right - 90, rc.bottom - 60, 80, 30,
                                  m_hwnd, (HMENU)1002, m_hInstance, nullptr);
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
            wchar_t buf[16]; swprintf_s(buf, L"%zu", i + 1);
            ListView_SetItemText(m_deviceList, idx, 2, buf);
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
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1001) {
            LOG_INFO(L"Settings button clicked");
            if (self && self->m_settingsWindow) {
                self->m_settingsWindow->Show();
            }
        } else if (LOWORD(wParam) == 1002) {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_APP + 2:
        if (self) self->RefreshDeviceList();
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
