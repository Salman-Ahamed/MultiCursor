#pragma once

#include "Core/Types.h"
#include "Core/CursorManager.h"
#include "Core/InputManager.h"
#include "Core/DeviceManager.h"
#include "Direct2DRenderer.h"
#include <atomic>
#include <thread>

class OverlayRenderer {
public:
    OverlayRenderer(Direct2DRenderer& renderer, CursorManager& cursorMgr,
                    InputManager& inputMgr, DeviceManager& devMgr);
    ~OverlayRenderer();

    void Start();
    void Stop();
    bool IsRunning() const { return m_running.load(); }

private:
    void RenderLoop();
    void WaitForFramePacing();
    void LateInputSample();
    RECT ComputeDirtyRect();

    Direct2DRenderer& m_renderer;
    CursorManager& m_cursorMgr;
    InputManager& m_inputMgr;
    DeviceManager& m_devMgr;

    std::thread m_thread;
    std::atomic<bool> m_running{ false };

    // Frame pacing
    using PFN_DCompositionWaitForCompositorClock = DWORD(WINAPI*)(UINT, const HANDLE*, DWORD);
    PFN_DCompositionWaitForCompositorClock m_waitForClock = nullptr;

    // Stat tracking
    uint64_t m_frameCount = 0;
    ULONGLONG m_lastStatTime = 0;
};
