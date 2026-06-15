#pragma once

#include "Types.h"
#include "EventBus.h"
#include <array>
#include <vector>
#include <unordered_map>
#include <mutex>

class CursorManager {
public:
    CursorManager(DeviceEventBus& devBus) : m_deviceBus(devBus) {
        m_deviceBus.Subscribe([this](const DeviceEvent& e) { OnDeviceEvent(e); });
    }

    UINT AddCursor(HANDLE hDevice, const std::wstring& label);
    void RemoveCursor(HANDLE hDevice);
    void ForcedButtonRelease(HANDLE hDevice);
    void UpdatePosition(UINT idx, LONG dx, LONG dy, bool absolute = false, LONG ax = 0, LONG ay = 0);
    void UpdateButton(UINT idx, int button, bool down);
    void TriggerRipple(POINT center);

    CursorState* GetCursor(UINT idx);
    UINT CursorCount() const { return (UINT)m_cursors.size(); }
    UINT LookupCursor(HANDLE hDevice) const;
    void SnapshotCursors(std::array<CursorState, kMaxDevices>& out) const;
    void SnapshotRipples(std::vector<RippleState>& out) const;

    void UpdateRipples(float dtMs);
    void ClearRipples();

private:
    void OnDeviceEvent(const DeviceEvent& e);

    mutable std::mutex m_mutex;
    std::array<CursorState, kMaxDevices> m_cursors;
    std::vector<RippleState> m_ripples;
    std::unordered_map<HANDLE, UINT> m_cursorMap;
    UINT m_nextSlot = 0;
    UINT m_colorIndex = 0;
    DeviceEventBus& m_deviceBus;
};
