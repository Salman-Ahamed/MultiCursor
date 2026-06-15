#include "MainWindow.h"
#include "Core/Logger.h"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

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
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
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
                              nullptr, nullptr, hInstance, nullptr);

    if (!m_hwnd) {
        LOG_ERROR(L"Failed to create MainWindow: %d", GetLastError());
        return false;
    }

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    LOG_INFO(L"MainWindow initialized");
    return true;
}

void MainWindow::Show(int nCmdShow) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, nCmdShow);
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

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
        } else if (LOWORD(wParam) == 1002) {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
