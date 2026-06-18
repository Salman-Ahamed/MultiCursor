#include "CursorManager.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

UINT CursorManager::AddCursor(HANDLE hDevice, const std::wstring& devicePath, const std::wstring& label) {
    std::lock_guard lock(m_mutex);
    if (m_nextSlot >= kMaxDevices) {
        LOG_ERROR(L"Max cursors reached");
        return (UINT)-1;
    }

    UINT idx = m_nextSlot++;
    auto& cursor = m_cursors[idx];
    cursor.label = label.empty() ? (L"Mouse " + std::to_wstring(idx + 1)) : label;
    cursor.dirty = true;

    m_cursorMap[hDevice] = idx;
    m_devicePaths[hDevice] = devicePath;

    LoadProfileForCursor(idx, devicePath);

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
    m_devicePaths.erase(hDevice);
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
        float speed = cursor.speedMultiplier;
        cursor.pos.x += (LONG)(dx * speed);
        cursor.pos.y += (LONG)(dy * speed);
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

void CursorManager::UpdateScroll(UINT idx, SHORT wheelDelta, SHORT wheelHDelta) {
    std::lock_guard lock(m_mutex);
    if (idx >= kMaxDevices) return;
    LOG_DEBUG(L"Cursor %u scroll: wheel=%d hwheel=%d", idx, wheelDelta, wheelHDelta);
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

void CursorManager::GetAllCursors(std::vector<CursorState>& out) const {
    std::lock_guard lock(m_mutex);
    out.clear();
    for (UINT i = 0; i < m_nextSlot; i++) {
        out.push_back(m_cursors[i]);
    }
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

void CursorManager::SetCursorColor(UINT idx, DWORD color) {
    std::lock_guard lock(m_mutex);
    if (idx >= kMaxDevices) return;
    m_cursors[idx].color = color;
    m_cursors[idx].dirty = true;
}

void CursorManager::SetCursorLabel(UINT idx, const std::wstring& label) {
    std::lock_guard lock(m_mutex);
    if (idx >= kMaxDevices) return;
    m_cursors[idx].label = label;
    m_cursors[idx].dirty = true;
}

void CursorManager::SetCursorVisible(UINT idx, bool visible) {
    std::lock_guard lock(m_mutex);
    if (idx >= kMaxDevices) return;
    m_cursors[idx].visible = visible;
    m_cursors[idx].dirty = true;
}

void CursorManager::SetCursorSpeed(UINT idx, float speedMultiplier) {
    std::lock_guard lock(m_mutex);
    if (idx >= kMaxDevices) return;
    m_cursors[idx].speedMultiplier = speedMultiplier;
}

void CursorManager::SetCursorSize(UINT idx, float size) {
    std::lock_guard lock(m_mutex);
    if (idx >= kMaxDevices) return;
    m_cursors[idx].cursorSize = size;
    m_cursors[idx].dirty = true;
}

void CursorManager::LoadProfileForCursor(UINT idx, const std::wstring& devicePath) {
    if (devicePath.empty()) return;
    CursorProfile profile = m_profileMgr.LoadProfile(devicePath);
    auto& cursor = m_cursors[idx];
    cursor.color = profile.color;
    cursor.label = profile.label;
    cursor.speedMultiplier = profile.speedMultiplier;
    cursor.cursorSize = profile.cursorSize;
    cursor.dirty = true;
    LOG_DEBUG(L"Loaded profile for cursor %u (color=0x%08X, speed=%.1f)", idx, profile.color, profile.speedMultiplier);
}

void CursorManager::SaveProfileForCursor(UINT idx, const std::wstring& devicePath) {
    if (devicePath.empty()) return;
    auto& cursor = m_cursors[idx];
    CursorProfile profile;
    profile.color = cursor.color;
    profile.label = cursor.label;
    profile.speedMultiplier = cursor.speedMultiplier;
    profile.cursorSize = cursor.cursorSize;
    m_profileMgr.SaveProfile(devicePath, profile);
}

void CursorManager::OnDeviceEvent(const DeviceEvent& e) {
    std::lock_guard lock(m_mutex);
    if (e.type == DeviceEvent::Removed) {
        for (auto it = m_cursorMap.begin(); it != m_cursorMap.end(); ) {
            if (it->first == e.device.hDevice) {
                auto& cursor = m_cursors[it->second];
                for (int i = 0; i < 5; i++) {
                    if (cursor.buttons[i]) {
                        cursor.buttons[i] = false;
                        cursor.dirty = true;
                        LOG_WARN(L"Forced button release for cursor %u, button %d", it->second, i);
                    }
                }
                m_cursors[it->second].visible = false;
                m_devicePaths.erase(it->first);
                it = m_cursorMap.erase(it);
            } else {
                ++it;
            }
        }
    }
}
