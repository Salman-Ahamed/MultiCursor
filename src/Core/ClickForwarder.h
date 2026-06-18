#pragma once

#include "Types.h"
#include <windows.h>

class ClickForwarder {
public:
    static void ForwardButtonDown(POINT cursorPos, USHORT buttonFlags);
    static void ForwardButtonUp(POINT cursorPos, USHORT buttonFlags);
};
