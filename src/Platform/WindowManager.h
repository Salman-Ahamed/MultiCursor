#pragma once

#include "Core/Types.h"
#include "Core/EventBus.h"
#include "Core/AppStateMachine.h"
#include <functional>

class WindowManager {
public:
    WindowManager(AppStateMachine& stateMachine);
    ~WindowManager();

    bool Initialize();
    void Shutdown();
    void Show();
    void Hide();
    HWND Handle() const { return m_hwnd; }

    void SetOverlayEnabled(bool enabled);
    bool IsOverlayEnabled() const { return m_overlayEnabled; }
    void HandleDpiChange(WPARAM wParam, LPARAM lParam);
    void HandleDisplayChange();
    void HandleSessionChange(WPARAM wParam);
    void HandlePowerBroadcast(WPARAM wParam);
    void SetDisplayChangeCallback(std::function<void(int, int)> cb) { m_displayChangeCb = std::move(cb); }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    bool CreateOverlayWindow();

    HWND m_hwnd = nullptr;
    bool m_overlayEnabled = true;
    AppStateMachine& m_stateMachine;
    std::function<void(int, int)> m_displayChangeCb;

    static WindowManager* s_instance;
};
