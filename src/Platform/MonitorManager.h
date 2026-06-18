#pragma once

#include "Core/Types.h"
#include <vector>

struct MonitorInfo {
    RECT rect = {};
    RECT workRect = {};
    UINT dpi = 96;
    bool isPrimary = false;
    std::wstring name;
};

class MonitorManager {
public:
    static std::vector<MonitorInfo> GetAllMonitors();
    static MonitorInfo GetPrimaryMonitor();
    static MonitorInfo GetMonitorAtPoint(int x, int y);
    static RECT VirtualDesktopRect();
    static POINT ConstrainToRect(POINT pt, const RECT& rect);
    static UINT GetDpiForMonitor(HMONITOR hMonitor);
    static float GetScaleFactor(HMONITOR hMonitor);

private:
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdc, LPRECT rect, LPARAM lParam);
};
