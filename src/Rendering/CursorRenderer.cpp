#include "CursorRenderer.h"

void CursorRenderer::Initialize(ID2D1DeviceContext* dc, IDWriteFactory* dwriteFactory) {
    D2D1_COLOR_F outlineColor = { 0.0f, 0.0f, 0.0f, 0.5f };
    dc->CreateSolidColorBrush(outlineColor, &m_outlineBrush);

    if (dwriteFactory) {
        dwriteFactory->CreateTextFormat(
            L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            11.0f, L"en-US", &m_textFormat);
        if (m_textFormat) {
            m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
}

void CursorRenderer::DrawCursor(ID2D1DeviceContext* dc, const CursorState& cursor, float opacity) {
    if (!dc || !cursor.visible) return;

    float radius = cursor.cursorSize;
    D2D1_POINT_2F center = D2D1::Point2F((FLOAT)cursor.pos.x, (FLOAT)cursor.pos.y);

    D2D1_COLOR_F fillColor;
    fillColor.r = ((cursor.color >> 16) & 0xFF) / 255.0f;
    fillColor.g = ((cursor.color >> 8) & 0xFF) / 255.0f;
    fillColor.b = (cursor.color & 0xFF) / 255.0f;
    fillColor.a = opacity;

    dc->CreateSolidColorBrush(fillColor, &m_brush);
    if (!m_brush) return;

    D2D1_ELLIPSE ellipse = D2D1::Ellipse(center, radius, radius);
    dc->FillEllipse(ellipse, m_brush.Get());

    if (m_outlineBrush) {
        dc->DrawEllipse(ellipse, m_outlineBrush.Get(), 1.5f);
    }

    if (m_textFormat && !cursor.label.empty()) {
        D2D1_RECT_F textRect = D2D1::RectF(
            center.x - 50, center.y + radius + 2,
            center.x + 50, center.y + radius + 18);
        dc->DrawTextW(cursor.label.c_str(), (UINT32)cursor.label.size(),
                      m_textFormat.Get(), textRect, m_brush.Get());
    }
}
