#include "OverlayRenderer.h"
#include "Core/Logger.h"
#include <chrono>
#include <algorithm>

using namespace std::chrono;

OverlayRenderer::OverlayRenderer(Direct2DRenderer& renderer, CursorManager& cursorMgr,
                                 InputManager& inputMgr, DeviceManager& devMgr)
    : m_renderer(renderer), m_cursorMgr(cursorMgr), m_inputMgr(inputMgr), m_devMgr(devMgr) {

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
    m_lastStatTime = GetTickCount64();
    m_frameCount = 0;

    std::array<CursorState, kMaxDevices> cursors;
    std::vector<RippleState> ripples;

    while (m_running.load()) {
        WaitForFramePacing();
        LateInputSample();

        RECT dirtyRect = ComputeDirtyRect();
        m_cursorMgr.UpdateRipples(16.0f);

        POINT offset = {};
        if (!m_renderer.BeginDraw(dirtyRect, offset)) {
            Sleep(100);
            continue;
        }

        // Thread-safe snapshot under mutex
        m_cursorMgr.SnapshotCursors(cursors);
        m_cursorMgr.SnapshotRipples(ripples);

        auto* dc = m_renderer.D2DContext();

        // Clear dirty rect to transparent
        D2D1_RECT_F clipRect = D2D1::RectF(
            (FLOAT)dirtyRect.left, (FLOAT)dirtyRect.top,
            (FLOAT)dirtyRect.right, (FLOAT)dirtyRect.bottom);
        dc->PushAxisAlignedClip(&clipRect, D2D1_ANTIALIAS_MODE_ALIASED);
        dc->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
        dc->PopAxisAlignedClip();

        // Draw cursors from snapshot
        for (UINT i = 0; i < kMaxDevices; i++) {
            auto& cursor = cursors[i];
            if (!cursor.visible) continue;
            if (!cursor.dirty) continue;

            D2D1_COLOR_F color;
            color.r = ((cursor.color >> 16) & 0xFF) / 255.0f;
            color.g = ((cursor.color >> 8) & 0xFF) / 255.0f;
            color.b = (cursor.color & 0xFF) / 255.0f;
            color.a = 0.8f;

            ComPtr<ID2D1SolidColorBrush> brush;
            dc->CreateSolidColorBrush(color, &brush);
            if (!brush) continue;

            D2D1_ELLIPSE ellipse = D2D1::Ellipse(
                D2D1::Point2F((FLOAT)cursor.pos.x, (FLOAT)cursor.pos.y),
                kCursorRadius, kCursorRadius);

            dc->FillEllipse(ellipse, brush.Get());

            D2D1_COLOR_F outlineColor = { 0.0f, 0.0f, 0.0f, 0.5f };
            ComPtr<ID2D1SolidColorBrush> outlineBrush;
            dc->CreateSolidColorBrush(outlineColor, &outlineBrush);
            if (outlineBrush) {
                dc->DrawEllipse(ellipse, outlineBrush.Get(), 1.5f);
            }
        }

        // Draw ripples from snapshot
        D2D1_COLOR_F rippleColor = { 1.0f, 1.0f, 1.0f, 0.6f };
        ComPtr<ID2D1SolidColorBrush> rippleBrush;
        dc->CreateSolidColorBrush(rippleColor, &rippleBrush);

        for (auto& ripple : ripples) {
            if (!ripple.active) continue;
            if (!rippleBrush) continue;

            rippleBrush->SetOpacity(ripple.opacity);

            D2D1_ELLIPSE re = D2D1::Ellipse(
                D2D1::Point2F((FLOAT)ripple.center.x, (FLOAT)ripple.center.y),
                ripple.radius, ripple.radius);
            dc->DrawEllipse(re, rippleBrush.Get(), ripple.strokeWidth);
        }

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

        if (state.buttonFlags & (RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_2_DOWN |
                                 RI_MOUSE_BUTTON_3_DOWN)) {
            auto* cursor = m_cursorMgr.GetCursor(cursorIdx);
            if (cursor) m_cursorMgr.TriggerRipple(cursor->pos);
        }
    }
}

RECT OverlayRenderer::ComputeDirtyRect() {
    RECT full = { 0, 0, m_renderer.SurfaceWidth(), m_renderer.SurfaceHeight() };
    return full;
}
