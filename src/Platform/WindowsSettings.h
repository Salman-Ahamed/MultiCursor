#pragma once

#include "Core/Types.h"

class WindowsSettings {
public:
    static bool SetMouseSpeed(int speed);
    static int GetMouseSpeed();

    static bool SetMouseThresholds(int threshold1, int threshold2);
    static bool SetEnhancePointerPrecision(bool enable);
    static bool SetDoubleClickTime(UINT ms);
    static UINT GetDoubleClickTime();

    static bool SetWheelScrollLines(int lines);
    static int GetWheelScrollLines();

    static bool SetSwapButtons(bool swap);
    static bool SetSnapToDefault(bool snap);
    static bool SetClickLock(bool lock);

    static bool ApplyProfile(const CursorProfile& profile);
};
