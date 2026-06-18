#include "OverlayRenderer.h"
#include "Core/Logger.h"
#include "Core/ClickForwarder.h"
#include <chrono>
#include <algorithm>

using namespace std::chrono;

OverlayRenderer::OverlayRenderer(Direct2DRenderer& renderer, CursorManager& cursorMgr,
                                 InputManager& inputMgr,
                                 SettingsManager& settings, AppStateMachine& stateMachine)
    : m_renderer(renderer), m_cursorMgr(cursorMgr), m_inputMgr(inputMgr),
      m_settings(settings), m_stateMachine(stateMachine) {

    HMODULE dcomp = LoadLibraryExW(L"dcomp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (dcomp) {
        m_waitForClock = (PFN_DCompositionWaitForCompositorClock)
            GetProcAddress(dcomp, "DCompositionWaitForCompositorClock");
        if (m_waitForClock) {
            LOG_INFO(L"DCompositionWaitForCompositorClock loaded (Win11+ frame pacing)");
        } else {
            LOG_INFO(L"DCompositionWaitForCompositorClock not available (Win10 fallback)");
        }
    }
}

OverlayRenderer::~OverlayRenderer() {
    Stop();
}

void OverlayRenderer::Start() {
    if (m_running.exchange(true)) return;
    m_thread = std::thread(&OverlayRenderer::RenderLoop, this);
    LOG_INFO(L"Render thread started");
}

void OverlayRenderer::Stop() {
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
    LOG_INFO(L"Render thread stopped");
}

void OverlayRenderer::RenderLoop() {
    SetThreadDescription(GetCurrentThread(), L"MultiCursor Render");
    m_lastStatTime = GetTickCount64();
    m_frameCount = 0;

    std::array<CursorState, kMaxDevices> cursors;

    while (m_running.load()) {
        if (m_suspended.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        WaitForFramePacing();
        LateInputSample();

        RECT dirtyRect = ComputeDirtyRect();
        m_clickEffects.Update(16.0f);

        POINT offset = {};
        if (!m_renderer.BeginDraw(dirtyRect, offset)) {
            LOG_WARN(L"BeginDraw failed, attempting device recovery");
            if (m_renderer.Recreate()) {
                m_modulesInitialized = false;
                LOG_INFO(L"Device chain recovered successfully");
                m_stateMachine.TransitionTo(AppState::DeviceLost);
                m_stateMachine.TransitionTo(AppState::Recovering);
                continue;
            }
            Sleep(100);
            continue;
        }

        m_cursorMgr.SnapshotCursors(cursors);

        auto* dc = m_renderer.D2DContext();
        auto* dwriteFactory = m_renderer.DWriteFactory();

        if (!m_modulesInitialized && dc) {
            m_cursorRenderer.Initialize(dc, dwriteFactory);
            m_clickEffects.Initialize(dc);
            m_modulesInitialized = true;
        }

        float cursorOpacity = m_settings.GetFloat("cursor_opacity", 0.8f);

        D2D1_RECT_F clipRect = D2D1::RectF(
            (FLOAT)dirtyRect.left, (FLOAT)dirtyRect.top,
            (FLOAT)dirtyRect.right, (FLOAT)dirtyRect.bottom);
        dc->PushAxisAlignedClip(&clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
        dc->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
        dc->PopAxisAlignedClip();

        for (UINT i = 0; i < kMaxDevices; i++) {
            auto& cursor = cursors[i];
            UpdateDirtyRect(cursor);
            if (!cursor.visible || !cursor.dirty) continue;
            m_cursorRenderer.DrawCursor(dc, cursor, cursorOpacity);
        }

        m_clickEffects.Draw(dc);

        m_renderer.EndDraw();
        m_renderer.Commit();

        m_frameCount++;
    }
}

void OverlayRenderer::WaitForFramePacing() {
    if (m_waitForClock) {
        DWORD result = m_waitForClock(0, nullptr, 100);
        if (result == WAIT_OBJECT_0) return;
    }

    static auto lastFrame = high_resolution_clock::now();
    auto now = high_resolution_clock::now();
    auto elapsed = duration_cast<milliseconds>(now - lastFrame).count();

    if (elapsed < 16) {
        Sleep((DWORD)(16 - elapsed));
    }

    lastFrame = high_resolution_clock::now();
}

void OverlayRenderer::LateInputSample() {
    RawInputFrame frame;
    while (m_inputMgr.TryConsume(frame)) {
        UINT cursorIdx = m_cursorMgr.LookupCursor(frame.hDevice);
        if (cursorIdx == (UINT)-1) continue;

        auto& state = frame.state;
        m_cursorMgr.UpdatePosition(cursorIdx, state.dx, state.dy, state.absolute, state.lx, state.ly);

        auto* cursor = m_cursorMgr.GetCursor(cursorIdx);
        if (!cursor) continue;

        POINT cursorPos = cursor->pos;

        if (state.wheelDelta != 0 || state.wheelHDelta != 0)
            m_cursorMgr.UpdateScroll(cursorIdx, state.wheelDelta, state.wheelHDelta);

        if (state.buttonFlags & RI_MOUSE_BUTTON_1_DOWN)
            m_clickEffects.Trigger(cursorPos, MouseButton::Left);
        if (state.buttonFlags & RI_MOUSE_BUTTON_2_DOWN)
            m_clickEffects.Trigger(cursorPos, MouseButton::Right);
        if (state.buttonFlags & RI_MOUSE_BUTTON_3_DOWN)
            m_clickEffects.Trigger(cursorPos, MouseButton::Middle);
        if (state.buttonFlags & RI_MOUSE_BUTTON_4_DOWN)
            m_clickEffects.Trigger(cursorPos, MouseButton::XButton1);
        if (state.buttonFlags & RI_MOUSE_BUTTON_5_DOWN)
            m_clickEffects.Trigger(cursorPos, MouseButton::XButton2);

        if (state.buttonFlags & (RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_2_DOWN |
                                 RI_MOUSE_BUTTON_3_DOWN | RI_MOUSE_BUTTON_4_DOWN |
                                 RI_MOUSE_BUTTON_5_DOWN)) {
            ClickForwarder::ForwardButtonDown(cursorPos, state.buttonFlags);
        }
        if (state.buttonFlags & (RI_MOUSE_BUTTON_1_UP | RI_MOUSE_BUTTON_2_UP |
                                 RI_MOUSE_BUTTON_3_UP | RI_MOUSE_BUTTON_4_UP |
                                 RI_MOUSE_BUTTON_5_UP)) {
            ClickForwarder::ForwardButtonUp(cursorPos, state.buttonFlags);
        }
    }
}

RECT OverlayRenderer::ComputeDirtyRect() {
    if (!m_hasDirtyRect) {
        return { 0, 0, m_renderer.SurfaceWidth(), m_renderer.SurfaceHeight() };
    }
    RECT r = m_dirtyRect;
    constexpr int pad = 64;
    r.left = (std::max)(0L, (long)(r.left - pad));
    r.top = (std::max)(0L, (long)(r.top - pad));
    r.right = (std::min)((int)m_renderer.SurfaceWidth(), (int)(r.right + pad));
    r.bottom = (std::min)((int)m_renderer.SurfaceHeight(), (int)(r.bottom + pad));
    m_hasDirtyRect = false;
    return r;
}

void OverlayRenderer::UpdateDirtyRect(const CursorState& cursor) {
    if (!cursor.visible || !cursor.dirty) return;
    RECT cr = { cursor.pos.x - 50, cursor.pos.y - 50,
                cursor.pos.x + 50, cursor.pos.y + 50 };
    if (m_hasDirtyRect) {
        UnionRect(&m_dirtyRect, &m_dirtyRect, &cr);
    } else {
        m_dirtyRect = cr;
        m_hasDirtyRect = true;
    }
}
