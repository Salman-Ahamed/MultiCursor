#pragma once

#include "Core/Types.h"
#include "Core/DeviceManager.h"
#include "Core/CursorManager.h"
#include "UI/TrayManager.h"
#include <windows.h>

class SettingsWindow;

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Initialize(HINSTANCE hInstance);
    void Show(int nCmdShow = SW_SHOW);
    HWND Handle() const { return m_hwnd; }

    void SetSettingsWindow(SettingsWindow* sw) { m_settingsWindow = sw; }
    void SetDeviceManager(DeviceManager* dm) { m_deviceMgr = dm; }
    void SetCursorManager(CursorManager* cm) { m_cursorMgr = cm; }
    void OnDeviceListUpdate();
    void RefreshDeviceList();
    TrayManager& Tray() { return m_tray; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void OnCreate();

    HWND m_hwnd = nullptr;
    HWND m_deviceList = nullptr;
    HWND m_statusBar = nullptr;
    HINSTANCE m_hInstance = nullptr;
    SettingsWindow* m_settingsWindow = nullptr;
    DeviceManager* m_deviceMgr = nullptr;
    CursorManager* m_cursorMgr = nullptr;
    HBRUSH m_darkBgBrush = nullptr;
    TrayManager m_tray;

    static MainWindow* s_instance;
};
