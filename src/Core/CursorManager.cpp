#include "CursorManager.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

UINT CursorManager::AddCursor(HANDLE hDevice, const std::wstring& label) {
    std::lock_guard lock(m_mutex);
    if (m_nextSlot >= kMaxDevices) {
        LOG_ERROR(L"Max cursors reached");
        return (UINT)-1;
    }

    UINT idx = m_nextSlot++;
    auto& cursor = m_cursors[idx];
    cursor.color = kCursorColors[m_colorIndex % kCursorColorCount];
    m_colorIndex++;
    cursor.label = label.empty() ? (L"Mouse " + std::to_wstring(idx + 1)) : label;
    cursor.dirty = true;

    m_cursorMap[hDevice] = idx;
    LOG_INFO(L"Added cursor %u: %s", idx, cursor.label.c_str());
    return idx;
}

void CursorManager::RemoveCursor(HANDLE hDevice) {
    std::lock_guard lock(m_mutex);
    auto it = m_cursorMap.find(hDevice);
    if (it == m_cursorMap.end()) return;

    UINT idx = it->second;
    m_cursors[idx].visible = false;
    m_cursorMap.erase(it);
    LOG_INFO(L"Removed cursor %u", idx);
}

void CursorManager::ForcedButtonRelease(HANDLE hDevice) {
    std::lock_guard lock(m_mutex);
    auto it = m_cursorMap.find(hDevice);
    if (it == m_cursorMap.end()) return;

    auto& cursor = m_cursors[it->second];
    for (int i = 0; i < 5; i++) {
        if (cursor.buttons[i]) {
            cursor.buttons[i] = false;
            cursor.dirty = true;
            LOG_WARN(L"Forced button release for cursor %u, button %d", it->second, i);
        }
    }
}

void CursorManager::UpdatePosition(UINT idx, LONG dx, LONG dy, bool absolute, LONG ax, LONG ay) {
    std::lock_guard lock(m_mutex);
    if (idx >= kMaxDevices || !m_cursors[idx].visible) return;
    auto& cursor = m_cursors[idx];

    if (absolute) {
        cursor.pos.x = ax;
        cursor.pos.y = ay;
    } else {
        cursor.pos.x += dx;
        cursor.pos.y += dy;
    }

    RECT virtualScreen;
    virtualScreen.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    virtualScreen.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    virtualScreen.right = virtualScreen.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    virtualScreen.bottom = virtualScreen.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    cursor.pos.x = (std::max)(virtualScreen.left, (std::min)(cursor.pos.x, virtualScreen.right - 1));
    cursor.pos.y = (std::max)(virtualScreen.top, (std::min)(cursor.pos.y, virtualScreen.bottom - 1));

    cursor.dirty = true;
}

void CursorManager::UpdateButton(UINT idx, int button, bool down) {
    std::lock_guard lock(m_mutex);
    if (idx >= kMaxDevices || button >= 5) return;
    m_cursors[idx].buttons[button] = down;
    m_cursors[idx].dirty = true;

    if (down) {
        TriggerRippleUnlocked(m_cursors[idx].pos);
    }
}

void CursorManager::TriggerRipple(POINT center) {
    std::lock_guard lock(m_mutex);
    TriggerRippleUnlocked(center);
}

void CursorManager::TriggerRippleUnlocked(POINT center) {
    for (auto& ripple : m_ripples) {
        if (!ripple.active) {
            ripple.center = center;
            ripple.radius = 0.0f;
            ripple.opacity = 1.0f;
            ripple.strokeWidth = 4.0f;
            ripple.elapsed = 0.0f;
            ripple.active = true;
            return;
        }
    }
    if (m_ripples.size() < kMaxRipples) {
        RippleState r;
        r.center = center;
        r.active = true;
        m_ripples.push_back(r);
    }
}

CursorState* CursorManager::GetCursor(UINT idx) {
    std::lock_guard lock(m_mutex);
    if (idx >= kMaxDevices) return nullptr;
    return &m_cursors[idx];
}

UINT CursorManager::LookupCursor(HANDLE hDevice) const {
    std::lock_guard lock(m_mutex);
    auto it = m_cursorMap.find(hDevice);
    return (it != m_cursorMap.end()) ? it->second : (UINT)-1;
}

void CursorManager::SnapshotCursors(std::array<CursorState, kMaxDevices>& out) const {
    std::lock_guard lock(m_mutex);
    out = m_cursors;
}

void CursorManager::SnapshotRipples(std::vector<RippleState>& out) const {
    std::lock_guard lock(m_mutex);
    out = m_ripples;
}

void CursorManager::UpdateRipples(float dtMs) {
    std::lock_guard lock(m_mutex);
    for (auto& ripple : m_ripples) {
        if (!ripple.active) continue;
        ripple.elapsed += dtMs;
        float t = ripple.elapsed / kRippleDurationMs;
        if (t >= 1.0f) {
            ripple.active = false;
            continue;
        }
        ripple.radius = t * kRippleMaxRadius;
        ripple.opacity = 1.0f - t;
        ripple.strokeWidth = 4.0f * (1.0f - t * 0.75f);
    }
}

void CursorManager::ClearRipples() {
    std::lock_guard lock(m_mutex);
    m_ripples.clear();
}

void CursorManager::OnDeviceEvent(const DeviceEvent& e) {
    std::lock_guard lock(m_mutex);
    if (e.type == DeviceEvent::Removed) {
        for (auto it = m_cursorMap.begin(); it != m_cursorMap.end(); ) {
            if (it->first == e.device.hDevice) {
                // Inline ForcedButtonRelease to avoid re-locking m_mutex (deadlock)
                auto& cursor = m_cursors[it->second];
                for (int i = 0; i < 5; i++) {
                    if (cursor.buttons[i]) {
                        cursor.buttons[i] = false;
                        cursor.dirty = true;
                        LOG_WARN(L"Forced button release for cursor %u, button %d", it->second, i);
                    }
                }
                m_cursors[it->second].visible = false;
                it = m_cursorMap.erase(it);
            } else {
                ++it;
            }
        }
    }
}
