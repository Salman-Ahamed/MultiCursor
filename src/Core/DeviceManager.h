#pragma once

#include "Types.h"
#include "EventBus.h"
#include <vector>
#include <unordered_map>

class DeviceManager {
public:
    DeviceManager(DeviceEventBus& bus) : m_eventBus(bus) {}

    bool Enumerate();
    bool OnDeviceArrival();
    bool OnDeviceRemoval();

    const DeviceInfo* GetDevice(HANDLE hDevice) const;
    const std::vector<DeviceInfo>& Devices() const { return m_devices; }
    UINT DeviceCount() const { return (UINT)m_devices.size(); }

private:
    bool ClassifyDevice(HANDLE hDevice, DeviceInfo& info);

    std::vector<DeviceInfo> m_devices;
    std::unordered_map<HANDLE, size_t> m_deviceMap;
    DeviceEventBus& m_eventBus;
    bool m_firstEnum = true;
};
