#pragma once

#include "Core/Types.h"
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class CursorRenderer {
public:
    CursorRenderer();

    bool Initialize(ID2D1DeviceContext* d2dContext, IDWriteFactory* dwriteFactory);
    void DrawCursor(ID2D1DeviceContext* dc, const CursorState& cursor);
    void DrawRipple(ID2D1DeviceContext* dc, const RippleState& ripple);

private:
    struct CursorResources {
        ComPtr<ID2D1SolidColorBrush> fillBrush;
        ComPtr<ID2D1SolidColorBrush> outlineBrush;
        ComPtr<IDWriteTextLayout> textLayout;
    };

    CursorResources m_cursorCache[kMaxDevices];
    ComPtr<IDWriteTextFormat> m_textFormat;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<ID2D1DeviceContext> m_d2dContext;
    bool m_initialized = false;
};
