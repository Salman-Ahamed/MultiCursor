#include "MonitorManager.h"
#include <shellscalingapi.h>

#pragma comment(lib, "shcore.lib")

struct EnumContext {
    std::vector<MonitorInfo>* monitors;
};

BOOL CALLBACK MonitorManager::MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT rect, LPARAM lParam) {
    auto* ctx = reinterpret_cast<EnumContext*>(lParam);
    MonitorInfo info;
    info.rect = *rect;

    MONITORINFOEXW mi = {};
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(hMonitor, &mi)) {
        info.workRect = mi.rcWork;
        info.name = mi.szDevice;
        info.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
    }

    info.dpi = GetDpiForMonitor(hMonitor);

    ctx->monitors->push_back(info);
    return TRUE;
}

std::vector<MonitorInfo> MonitorManager::GetAllMonitors() {
    std::vector<MonitorInfo> monitors;
    EnumContext ctx = { &monitors };
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&ctx));
    return monitors;
}

MonitorInfo MonitorManager::GetPrimaryMonitor() {
    auto all = GetAllMonitors();
    for (auto& m : all) {
        if (m.isPrimary) return m;
    }
    if (!all.empty()) return all[0];
    MonitorInfo fallback;
    fallback.rect = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    fallback.workRect = fallback.rect;
    return fallback;
}

MonitorInfo MonitorManager::GetMonitorAtPoint(int x, int y) {
    POINT pt = { x, y };
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEXW mi = {};
    mi.cbSize = sizeof(mi);
    MonitorInfo info;
    if (GetMonitorInfoW(hMonitor, &mi)) {
        info.rect = mi.rcMonitor;
        info.workRect = mi.rcWork;
        info.name = mi.szDevice;
        info.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
    }
    info.dpi = GetDpiForMonitor(hMonitor);
    return info;
}

RECT MonitorManager::VirtualDesktopRect() {
    return {
        GetSystemMetrics(SM_XVIRTUALSCREEN),
        GetSystemMetrics(SM_YVIRTUALSCREEN),
        GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN),
        GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN)
    };
}

POINT MonitorManager::ConstrainToRect(POINT pt, const RECT& rect) {
    pt.x = (std::max)(rect.left, (std::min)(pt.x, rect.right - 1));
    pt.y = (std::max)(rect.top, (std::min)(pt.y, rect.bottom - 1));
    return pt;
}

UINT MonitorManager::GetDpiForMonitor(HMONITOR hMonitor) {
    UINT dpiX = 96, dpiY = 96;
    ::GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    return dpiX;
}

float MonitorManager::GetScaleFactor(HMONITOR hMonitor) {
    return GetDpiForMonitor(hMonitor) / 96.0f;
}
