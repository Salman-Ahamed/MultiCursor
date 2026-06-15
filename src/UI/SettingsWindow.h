#pragma once

#include "Core/Types.h"
#include <windows.h>

class SettingsWindow {
public:
    SettingsWindow();
    ~SettingsWindow();

    bool Initialize(HINSTANCE hInstance, HWND parent);
    void Show();

    HWND Handle() const { return m_hwnd; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void OnCreate();

    HWND m_hwnd = nullptr;
    HWND m_parent = nullptr;
    static SettingsWindow* s_instance;
};
