#include "ClickForwarder.h"
#include "Logger.h"

static void SendMouseInput(DWORD dwFlags, POINT cursorPos, DWORD mouseData = 0) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = (cursorPos.x * 65536) / GetSystemMetrics(SM_CXVIRTUALSCREEN);
    input.mi.dy = (cursorPos.y * 65536) / GetSystemMetrics(SM_CYVIRTUALSCREEN);
    input.mi.dwFlags = dwFlags | MOUSEEVENTF_ABSOLUTE;
    input.mi.dwExtraInfo = kSendInputMagic;
    input.mi.mouseData = mouseData;

    UINT sent = SendInput(1, &input, sizeof(INPUT));
    if (sent != 1) {
        LOG_WARN(L"SendInput failed: %d", GetLastError());
    }
}

void ClickForwarder::ForwardButtonDown(POINT cursorPos, USHORT buttonFlags) {
    bool hasStandard = false;
    DWORD stdFlags = 0;
    if (buttonFlags & RI_MOUSE_BUTTON_1_DOWN) { stdFlags |= MOUSEEVENTF_LEFTDOWN; hasStandard = true; }
    if (buttonFlags & RI_MOUSE_BUTTON_2_DOWN) { stdFlags |= MOUSEEVENTF_RIGHTDOWN; hasStandard = true; }
    if (buttonFlags & RI_MOUSE_BUTTON_3_DOWN) { stdFlags |= MOUSEEVENTF_MIDDLEDOWN; hasStandard = true; }

    bool xb1 = (buttonFlags & RI_MOUSE_BUTTON_4_DOWN) != 0;
    bool xb2 = (buttonFlags & RI_MOUSE_BUTTON_5_DOWN) != 0;

    // Standard buttons in one call
    if (hasStandard) SendMouseInput(stdFlags, cursorPos);

    // X buttons need separate calls for correct mouseData
    if (xb1) SendMouseInput(MOUSEEVENTF_XDOWN, cursorPos, XBUTTON1);
    if (xb2) SendMouseInput(MOUSEEVENTF_XDOWN, cursorPos, XBUTTON2);
}

void ClickForwarder::ForwardButtonUp(POINT cursorPos, USHORT buttonFlags) {
    bool hasStandard = false;
    DWORD stdFlags = 0;
    if (buttonFlags & RI_MOUSE_BUTTON_1_UP) { stdFlags |= MOUSEEVENTF_LEFTUP; hasStandard = true; }
    if (buttonFlags & RI_MOUSE_BUTTON_2_UP) { stdFlags |= MOUSEEVENTF_RIGHTUP; hasStandard = true; }
    if (buttonFlags & RI_MOUSE_BUTTON_3_UP) { stdFlags |= MOUSEEVENTF_MIDDLEUP; hasStandard = true; }

    bool xb1 = (buttonFlags & RI_MOUSE_BUTTON_4_UP) != 0;
    bool xb2 = (buttonFlags & RI_MOUSE_BUTTON_5_UP) != 0;

    if (hasStandard) SendMouseInput(stdFlags, cursorPos);

    if (xb1) SendMouseInput(MOUSEEVENTF_XUP, cursorPos, XBUTTON1);
    if (xb2) SendMouseInput(MOUSEEVENTF_XUP, cursorPos, XBUTTON2);
}
