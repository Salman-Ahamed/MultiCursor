#pragma once

#include "Core/Types.h"
#include <vector>
#include <string>

class DeviceEnumerator {
public:
    static std::vector<RAWINPUTDEVICELIST> EnumerateRawInputDevices(DWORD type);
    static std::wstring GetDeviceName(HANDLE hDevice);
    static bool GetDeviceInfo(HANDLE hDevice, RID_DEVICE_INFO& info);
    static std::wstring GetDevicePathHash(const std::wstring& devicePath);
    static DeviceType ClassifyDevice(HANDLE hDevice, DWORD dwType);
    static bool RegisterForDeviceNotifications(HWND hwnd);

    static std::wstring GetDevicePath(HANDLE hDevice);
};
