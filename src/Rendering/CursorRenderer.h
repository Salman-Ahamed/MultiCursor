#pragma once

#include "Core/Types.h"
#include <wrl/client.h>
#include <d2d1_1.h>
#include <dwrite.h>

using Microsoft::WRL::ComPtr;

class CursorRenderer {
public:
    CursorRenderer() = default;
    ~CursorRenderer() = default;

    void Initialize(ID2D1DeviceContext* dc, IDWriteFactory* dwriteFactory);
    void DrawCursor(ID2D1DeviceContext* dc, const CursorState& cursor, float opacity);

private:
    ComPtr<ID2D1SolidColorBrush> m_brush;
    ComPtr<ID2D1SolidColorBrush> m_outlineBrush;
    ComPtr<IDWriteTextFormat> m_textFormat;
};
