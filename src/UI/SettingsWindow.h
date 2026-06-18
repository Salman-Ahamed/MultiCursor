#pragma once

#include "Core/Types.h"
#include "Core/SettingsManager.h"
#include "Core/CursorManager.h"
#include <windows.h>

class SettingsWindow {
public:
    SettingsWindow();
    ~SettingsWindow();

    bool Initialize(HINSTANCE hInstance, HWND parent, SettingsManager& settings);
    void Show();
    void SetCursorManager(CursorManager* cm) { m_cursorMgr = cm; }

    HWND Handle() const { return m_hwnd; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void OnCreate();
    void LoadSettings();
    void SaveSettings();

    HWND m_hwnd = nullptr;
    HWND m_parent = nullptr;
    HWND m_cursorSizeSlider = nullptr;
    HWND m_cursorSizeLabel = nullptr;
    HWND m_overlayCheck = nullptr;
    HWND m_labelsCheck = nullptr;
    HWND m_opacitySlider = nullptr;
    HWND m_logLevelCombo = nullptr;
    HWND m_speedSlider = nullptr;
    HWND m_speedLabel = nullptr;
    HWND m_deviceCombo = nullptr;
    SettingsManager* m_settings = nullptr;
    CursorManager* m_cursorMgr = nullptr;
    HBRUSH m_darkBgBrush = nullptr;
    static SettingsWindow* s_instance;
};
