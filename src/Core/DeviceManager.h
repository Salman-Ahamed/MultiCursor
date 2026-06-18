#pragma once

#include "Types.h"
#include "EventBus.h"
#include <vector>
#include <unordered_map>
#include <mutex>

class DeviceManager {
public:
    DeviceManager(DeviceEventBus& bus) : m_eventBus(bus) {}

    bool Enumerate();
    bool OnDeviceArrival();
    bool OnDeviceRemoval();

    const DeviceInfo* GetDevice(HANDLE hDevice) const;
    const std::vector<DeviceInfo>& Devices() const;
    UINT DeviceCount() const;

private:
    bool EnumerateUnlocked();
    bool ClassifyDevice(HANDLE hDevice, DeviceInfo& info, DWORD dwType);

    std::vector<DeviceInfo> m_devices;
    std::unordered_map<HANDLE, size_t> m_deviceMap;
    DeviceEventBus& m_eventBus;
    bool m_firstEnum = true;
    mutable std::mutex m_mutex;
};
