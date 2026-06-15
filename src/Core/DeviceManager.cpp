#include "DeviceManager.h"
#include "Logger.h"
#include <vector>
#include <set>
#include <algorithm>
#include <winternl.h>
#include <hidusage.h>
#include <hidpi.h>

#pragma comment(lib, "hid.lib")

bool DeviceManager::Enumerate() {
    UINT count = 0;
    if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1) {
        LOG_ERROR(L"GetRawInputDeviceList failed: %d", GetLastError());
        return false;
    }

    std::vector<RAWINPUTDEVICELIST> list(count);
    UINT actual = GetRawInputDeviceList(list.data(), &count, sizeof(RAWINPUTDEVICELIST));
    if (actual == (UINT)-1) return false;

    std::vector<DeviceInfo> newDevices;

    for (UINT i = 0; i < actual; i++) {
        if (list[i].dwType != RIM_TYPEMOUSE && list[i].dwType != RIM_TYPEHID)
            continue;

        DeviceInfo info;
        info.hDevice = list[i].hDevice;

        UINT nameLen = 0;
        if (GetRawInputDeviceInfoW(list[i].hDevice, RIDI_DEVICENAME, nullptr, &nameLen) == (UINT)-1)
            continue;
        info.name.resize(nameLen);
        GetRawInputDeviceInfoW(list[i].hDevice, RIDI_DEVICENAME, &info.name[0], &nameLen);
        info.name.resize(wcslen(info.name.c_str()));
        info.path = info.name;

        if (!ClassifyDevice(list[i].hDevice, info))
            continue;

        if (info.type == DeviceType::Mouse)
            info.isMouse = true;

        newDevices.push_back(std::move(info));
    }

    std::set<std::wstring> ptpGroups;
    for (auto& d : newDevices) {
        if (d.type == DeviceType::Touchpad || d.type == DeviceType::TouchScreen || d.type == DeviceType::Pen) {
            auto colonPos = d.path.rfind(L'#');
            auto prefix = (colonPos != std::wstring::npos) ? d.path.substr(0, colonPos) : d.path;
            ptpGroups.insert(prefix);
        }
    }

    m_devices.clear();
    m_deviceMap.clear();
    for (auto& d : newDevices) {
        auto colonPos = d.path.rfind(L'#');
        auto prefix = (colonPos != std::wstring::npos) ? d.path.substr(0, colonPos) : d.path;
        if (ptpGroups.count(prefix) && d.type == DeviceType::Mouse) {
            LOG_INFO(L"Skipping PTP mouse-emulation TLC: %s", d.path.c_str());
            continue;
        }
        if (d.isMouse) {
            m_deviceMap[d.hDevice] = m_devices.size();
            m_devices.push_back(std::move(d));
        }
    }

    if (!m_firstEnum) {
        DeviceEvent ev{ DeviceEvent::Changed, {} };
        m_eventBus.Publish(ev);
    }
    m_firstEnum = false;

    LOG_INFO(L"Enumerated %zu mouse devices", m_devices.size());
    return true;
}

bool DeviceManager::OnDeviceArrival() {
    return Enumerate();
}

bool DeviceManager::OnDeviceRemoval() {
    return Enumerate();
}

const DeviceInfo* DeviceManager::GetDevice(HANDLE hDevice) const {
    auto it = m_deviceMap.find(hDevice);
    if (it != m_deviceMap.end())
        return &m_devices[it->second];
    return nullptr;
}

bool DeviceManager::ClassifyDevice(HANDLE hDevice, DeviceInfo& info) {
    UINT size = 0;
    if (GetRawInputDeviceInfoW(hDevice, RIDI_PREPARSEDDATA, nullptr, &size) == (UINT)-1 || size == 0) {
        if (info.path.find(L"SYNA") != std::wstring::npos ||
            info.path.find(L"ELAN") != std::wstring::npos ||
            info.path.find(L"DLL0") != std::wstring::npos) {
            info.type = DeviceType::Touchpad;
            return true;
        }
        return false;
    }

    std::vector<uint8_t> buf(size);
    if (GetRawInputDeviceInfoW(hDevice, RIDI_PREPARSEDDATA, buf.data(), &size) == (UINT)-1)
        return false;

    HIDP_CAPS caps;
    auto preparsed = reinterpret_cast<PHIDP_PREPARSED_DATA>(buf.data());
    if (HidP_GetCaps(preparsed, &caps) != HIDP_STATUS_SUCCESS)
        return false;

    if (caps.UsagePage == 0x0D) {
        switch (caps.Usage) {
        case 0x01: case 0x02: info.type = DeviceType::Pen; break;
        case 0x04: info.type = DeviceType::TouchScreen; break;
        case 0x05: info.type = DeviceType::Touchpad; break;
        default: info.type = DeviceType::Other; break;
        }
        return true;
    }

    if (caps.UsagePage == 0x01) {
        if (caps.Usage == 0x02) {
            info.type = DeviceType::Mouse;
            return true;
        }
        if (caps.Usage == 0x06) {
            info.type = DeviceType::Keyboard;
            return true;
        }
    }

    info.type = DeviceType::Other;
    return true;
}
