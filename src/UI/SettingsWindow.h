#pragma once

#include "Core/Types.h"
#include "Core/SettingsManager.h"
#include <windows.h>

class SettingsWindow {
public:
    SettingsWindow();
    ~SettingsWindow();

    bool Initialize(HINSTANCE hInstance, HWND parent, SettingsManager& settings);
    void Show();

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
    SettingsManager* m_settings = nullptr;
    static SettingsWindow* s_instance;
};
