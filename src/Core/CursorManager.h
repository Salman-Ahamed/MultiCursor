#pragma once

#include "Types.h"
#include "EventBus.h"
#include "ProfileManager.h"
#include <array>
#include <vector>
#include <unordered_map>
#include <mutex>

class CursorManager {
public:
    CursorManager(DeviceEventBus& devBus, ProfileManager& profileMgr)
        : m_deviceBus(devBus), m_profileMgr(profileMgr) {
        m_deviceBus.Subscribe([this](const DeviceEvent& e) { OnDeviceEvent(e); });
    }

    UINT AddCursor(HANDLE hDevice, const std::wstring& devicePath, const std::wstring& label);
    void RemoveCursor(HANDLE hDevice);
    void ForcedButtonRelease(HANDLE hDevice);
    void UpdatePosition(UINT idx, LONG dx, LONG dy, bool absolute = false, LONG ax = 0, LONG ay = 0);
    void UpdateButton(UINT idx, int button, bool down);
    void UpdateScroll(UINT idx, SHORT wheelDelta, SHORT wheelHDelta);
    void TriggerRipple(POINT center);
    void TriggerRippleUnlocked(POINT center);

    CursorState* GetCursor(UINT idx);
    UINT CursorCount() const { return (UINT)m_nextSlot; }
    UINT LookupCursor(HANDLE hDevice) const;
    void SnapshotCursors(std::array<CursorState, kMaxDevices>& out) const;
    void SnapshotRipples(std::vector<RippleState>& out) const;
    void GetAllCursors(std::vector<CursorState>& out) const;

    void UpdateRipples(float dtMs);
    void ClearRipples();

    void SetCursorColor(UINT idx, DWORD color);
    void SetCursorLabel(UINT idx, const std::wstring& label);
    void SetCursorVisible(UINT idx, bool visible);
    void SetCursorSpeed(UINT idx, float speedMultiplier);
    void SetCursorSize(UINT idx, float size);

    void LoadProfileForCursor(UINT idx, const std::wstring& devicePath);
    void SaveProfileForCursor(UINT idx, const std::wstring& devicePath);

private:
    void OnDeviceEvent(const DeviceEvent& e);

    mutable std::mutex m_mutex;
    std::array<CursorState, kMaxDevices> m_cursors;
    std::vector<RippleState> m_ripples;
    std::unordered_map<HANDLE, UINT> m_cursorMap;
    std::unordered_map<HANDLE, std::wstring> m_devicePaths;
    UINT m_nextSlot = 0;
    UINT m_colorIndex = 0;
    DeviceEventBus& m_deviceBus;
    ProfileManager& m_profileMgr;
};
