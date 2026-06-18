#include "DeviceEnumerator.h"
#include "Core/Logger.h"
#include <winternl.h>
#include <hidusage.h>
#include <hidpi.h>
#include <dbt.h>
#include <functional>
#include <initguid.h>
#include <hidclass.h>

// {378DE44C-56EF-11D1-BC8C-00A0C91405DD}
const GUID MY_GUID_DEVINTERFACE_MOUSE = {0x378DE44C, 0x56EF, 0x11D1, {0xBC, 0x8C, 0x00, 0xA0, 0xC9, 0x14, 0x05, 0xDD}};

#pragma comment(lib, "hid.lib")

std::vector<RAWINPUTDEVICELIST> DeviceEnumerator::EnumerateRawInputDevices(DWORD type) {
    UINT count = 0;
    if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1) {
        return {};
    }

    std::vector<RAWINPUTDEVICELIST> list(count);
    UINT actual = GetRawInputDeviceList(list.data(), &count, sizeof(RAWINPUTDEVICELIST));
    if (actual == (UINT)-1) return {};

    std::vector<RAWINPUTDEVICELIST> result;
    for (UINT i = 0; i < actual; i++) {
        if (list[i].dwType == type) {
            result.push_back(list[i]);
        }
    }
    return result;
}

std::wstring DeviceEnumerator::GetDeviceName(HANDLE hDevice) {
    UINT len = 0;
    if (GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, nullptr, &len) == (UINT)-1)
        return {};

    std::wstring name(len, L'\0');
    GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, &name[0], &len);
    name.resize(wcslen(name.c_str()));
    return name;
}

std::wstring DeviceEnumerator::GetDevicePath(HANDLE hDevice) {
    return GetDeviceName(hDevice);
}

bool DeviceEnumerator::GetDeviceInfo(HANDLE hDevice, RID_DEVICE_INFO& info) {
    info.cbSize = sizeof(RID_DEVICE_INFO);
    UINT size = sizeof(RID_DEVICE_INFO);
    return GetRawInputDeviceInfoW(hDevice, RIDI_DEVICEINFO, &info, &size) != (UINT)-1;
}

std::wstring DeviceEnumerator::GetDevicePathHash(const std::wstring& devicePath) {
    std::hash<std::wstring> hasher;
    wchar_t buf[32];
    swprintf_s(buf, L"%016zX", hasher(devicePath));
    return buf;
}

DeviceType DeviceEnumerator::ClassifyDevice(HANDLE hDevice, DWORD dwType) {
    if (dwType == RIM_TYPEMOUSE) {
        return DeviceType::Mouse;
    }
    if (dwType == RIM_TYPEKEYBOARD) {
        return DeviceType::Keyboard;
    }

    UINT size = 0;
    std::wstring path = GetDevicePath(hDevice);
    if (GetRawInputDeviceInfoW(hDevice, RIDI_PREPARSEDDATA, nullptr, &size) == (UINT)-1 || size == 0) {
        if (path.find(L"SYNA") != std::wstring::npos ||
            path.find(L"ELAN") != std::wstring::npos ||
            path.find(L"DLL0") != std::wstring::npos) {
            return DeviceType::Touchpad;
        }
        return DeviceType::Other;
    }

    std::vector<uint8_t> buf(size);
    if (GetRawInputDeviceInfoW(hDevice, RIDI_PREPARSEDDATA, buf.data(), &size) == (UINT)-1)
        return DeviceType::Other;

    HIDP_CAPS caps;
    auto preparsed = reinterpret_cast<PHIDP_PREPARSED_DATA>(buf.data());
    if (HidP_GetCaps(preparsed, &caps) != HIDP_STATUS_SUCCESS)
        return DeviceType::Other;

    if (caps.UsagePage == 0x0D) {
        switch (caps.Usage) {
        case 0x01: case 0x02: return DeviceType::Pen;
        case 0x04: return DeviceType::TouchScreen;
        case 0x05: return DeviceType::Touchpad;
        default: return DeviceType::Other;
        }
    }

    if (caps.UsagePage == 0x01) {
        if (caps.Usage == 0x02) return DeviceType::Mouse;
        if (caps.Usage == 0x06) return DeviceType::Keyboard;
    }

    return DeviceType::Other;
}

bool DeviceEnumerator::RegisterForDeviceNotifications(HWND hwnd) {
    DEV_BROADCAST_DEVICEINTERFACE_W filter = {};
    filter.dbcc_size = sizeof(filter);
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    filter.dbcc_classguid = MY_GUID_DEVINTERFACE_MOUSE;

    HDEVNOTIFY notify = RegisterDeviceNotificationW(hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
    return notify != nullptr;
}
