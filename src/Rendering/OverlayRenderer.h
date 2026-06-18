#pragma once

#include "Core/Types.h"
#include "Core/CursorManager.h"
#include "Core/InputManager.h"
#include "Core/SettingsManager.h"
#include "Core/AppStateMachine.h"

#include "Direct2DRenderer.h"
#include "CursorRenderer.h"
#include "ClickEffects.h"
#include <atomic>
#include <thread>

class OverlayRenderer {
public:
    OverlayRenderer(Direct2DRenderer& renderer, CursorManager& cursorMgr,
                    InputManager& inputMgr,
                    SettingsManager& settings, AppStateMachine& stateMachine);
    ~OverlayRenderer();

    void Start();
    void Stop();
    void Suspend() { m_suspended.store(true); }
    void Resume() { m_suspended.store(false); }
    bool IsRunning() const { return m_running.load(); }

private:
    void RenderLoop();
    void WaitForFramePacing();
    void LateInputSample();
    RECT ComputeDirtyRect();
    void UpdateDirtyRect(const CursorState& cursor);

    Direct2DRenderer& m_renderer;
    CursorManager& m_cursorMgr;
    InputManager& m_inputMgr;
    SettingsManager& m_settings;
    AppStateMachine& m_stateMachine;

    std::thread m_thread;
    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_suspended{ false };

    CursorRenderer m_cursorRenderer;
    ClickEffects m_clickEffects;
    bool m_modulesInitialized = false;

    // Frame pacing
    using PFN_DCompositionWaitForCompositorClock = DWORD(WINAPI*)(UINT, const HANDLE*, DWORD);
    PFN_DCompositionWaitForCompositorClock m_waitForClock = nullptr;

    // Dirty rect tracking
    RECT m_dirtyRect = {};
    bool m_hasDirtyRect = false;

    // Stat tracking
    uint64_t m_frameCount = 0;
    ULONGLONG m_lastStatTime = 0;
};
