#pragma once

#include "Core/Types.h"
#include <windows.h>

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Initialize(HINSTANCE hInstance);
    void Show(int nCmdShow = SW_SHOW);
    HWND Handle() const { return m_hwnd; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void OnCreate();
    void OnDeviceListUpdate();

    HWND m_hwnd = nullptr;
    HWND m_deviceList = nullptr;
    HINSTANCE m_hInstance = nullptr;

    static MainWindow* s_instance;
};
