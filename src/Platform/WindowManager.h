#pragma once

#include "Core/Types.h"
#include "Core/EventBus.h"

class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    bool Initialize();
    void Shutdown();
    void Show();
    void Hide();
    HWND Handle() const { return m_hwnd; }

    void SetOverlayEnabled(bool enabled);
    void HandleDpiChange(WPARAM wParam, LPARAM lParam);
    void HandleSessionChange(WPARAM wParam);
    void HandlePowerBroadcast(WPARAM wParam);

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    bool CreateOverlayWindow();

    HWND m_hwnd = nullptr;
    bool m_overlayEnabled = true;
    bool m_dcompReady = false;

    static WindowManager* s_instance;
};
