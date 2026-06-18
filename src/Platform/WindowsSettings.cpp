#include "WindowsSettings.h"
#include "Core/Logger.h"

#ifndef SPI_SNAPTODEFBUTTON
#define SPI_SNAPTODEFBUTTON 0x005F
#endif
#ifndef SPI_SETCLICKLOCK
#define SPI_SETCLICKLOCK 0x0061
#endif

bool WindowsSettings::SetMouseSpeed(int speed) {
    if (speed < 1 || speed > 20) return false;
    return SystemParametersInfoW(SPI_SETMOUSESPEED, (UINT)speed, nullptr, SPIF_SENDCHANGE) != FALSE;
}

int WindowsSettings::GetMouseSpeed() {
    UINT speed = 10;
    SystemParametersInfoW(SPI_GETMOUSESPEED, 0, &speed, 0);
    return (int)speed;
}

bool WindowsSettings::SetMouseThresholds(int threshold1, int threshold2) {
    int params[3] = { threshold1, threshold2, 0 };
    return SystemParametersInfoW(SPI_SETMOUSE, 0, params, SPIF_SENDCHANGE) != FALSE;
}

static bool GetMouseParams(int& threshold1, int& threshold2, bool& enhance) {
    int params[3] = {};
    if (!SystemParametersInfoW(SPI_GETMOUSE, 0, params, 0)) return false;
    threshold1 = params[0];
    threshold2 = params[1];
    enhance = (params[2] != 0);
    return true;
}

bool WindowsSettings::SetEnhancePointerPrecision(bool enable) {
    int t1 = 0, t2 = 0;
    bool existing = false;
    GetMouseParams(t1, t2, existing);
    int params[3] = { t1, t2, enable ? 1 : 0 };
    return SystemParametersInfoW(SPI_SETMOUSE, 0, params, SPIF_SENDCHANGE) != FALSE;
}

bool WindowsSettings::SetDoubleClickTime(UINT ms) {
    if (ms < 50 || ms > 5000) return false;
    return ::SetDoubleClickTime(ms) != FALSE;
}

UINT WindowsSettings::GetDoubleClickTime() {
    return ::GetDoubleClickTime();
}

bool WindowsSettings::SetWheelScrollLines(int lines) {
    if (lines < 0 || lines > 100) return false;
    return SystemParametersInfoW(SPI_SETWHEELSCROLLLINES, (UINT)lines, nullptr, SPIF_SENDCHANGE) != FALSE;
}

int WindowsSettings::GetWheelScrollLines() {
    UINT lines = 3;
    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
    return (int)lines;
}

bool WindowsSettings::SetSwapButtons(bool swap) {
    return SwapMouseButton(swap ? TRUE : FALSE) != FALSE;
}

bool WindowsSettings::SetSnapToDefault(bool snap) {
    return SystemParametersInfoW(SPI_SNAPTODEFBUTTON, snap ? 1 : 0, nullptr, SPIF_SENDCHANGE) != FALSE;
}

bool WindowsSettings::SetClickLock(bool lock) {
    return SystemParametersInfoW(SPI_SETCLICKLOCK, lock ? 1 : 0, nullptr, SPIF_SENDCHANGE) != FALSE;
}

bool WindowsSettings::ApplyProfile(const CursorProfile& profile) {
    bool ok = true;
    ok = SetMouseSpeed((int)(profile.speedMultiplier * 10.0f)) || ok;
    ok = SetEnhancePointerPrecision(profile.enhancePrecision) || ok;
    ok = SetDoubleClickTime((UINT)profile.doubleClickSpeed) || ok;
    ok = SetWheelScrollLines(profile.scrollLines) || ok;
    ok = SetSnapToDefault(profile.snapToDefault) || ok;
    ok = SetClickLock(profile.clickLock) || ok;
    return ok;
}
