#include "CursorRenderer.h"
#include "Core/Logger.h"

CursorRenderer::CursorRenderer() = default;

bool CursorRenderer::Initialize(ID2D1DeviceContext* d2dContext, IDWriteFactory* dwriteFactory) {
    m_d2dContext = d2dContext;
    m_dwriteFactory = dwriteFactory;

    HRESULT hr = dwriteFactory->CreateTextFormat(
        L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        11.0f, L"en-US",
        &m_textFormat);

    if (FAILED(hr)) {
        LOG_ERROR(L"CreateTextFormat failed: 0x%08X", hr);
        return false;
    }

    m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    m_initialized = true;
    return true;
}

void CursorRenderer::DrawCursor(ID2D1DeviceContext* dc, const CursorState& cursor) {
    if (!m_initialized || !cursor.visible) return;

    // Convert DWORD color to D2D1_COLOR_F
    D2D1_COLOR_F fillColor;
    fillColor.r = ((cursor.color >> 16) & 0xFF) / 255.0f;
    fillColor.g = ((cursor.color >> 8) & 0xFF) / 255.0f;
    fillColor.b = (cursor.color & 0xFF) / 255.0f;
    fillColor.a = 0.8f;

    ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = dc->CreateSolidColorBrush(fillColor, &brush);
    if (FAILED(hr)) return;

    D2D1_ELLIPSE ellipse = D2D1::Ellipse(
        D2D1::Point2F((FLOAT)cursor.pos.x, (FLOAT)cursor.pos.y),
        kCursorRadius, kCursorRadius);

    dc->FillEllipse(ellipse, brush.Get());

    // Outline
    D2D1_COLOR_F outlineColor = { 0.0f, 0.0f, 0.0f, 0.5f };
    ComPtr<ID2D1SolidColorBrush> outlineBrush;
    if (SUCCEEDED(dc->CreateSolidColorBrush(outlineColor, &outlineBrush))) {
        dc->DrawEllipse(ellipse, outlineBrush.Get(), 1.5f);
    }

    // Label
    if (!cursor.label.empty() && m_textFormat) {
        auto label = cursor.label.c_str();
        UINT32 len = (UINT32)cursor.label.size();

        static_cast<ID2D1RenderTarget*>(dc)->DrawTextW(
            label, len, m_textFormat.Get(),
            D2D1::RectF(
                (FLOAT)cursor.pos.x - 50,
                (FLOAT)cursor.pos.y + kCursorRadius + 2,
                (FLOAT)cursor.pos.x + 50,
                (FLOAT)cursor.pos.y + kCursorRadius + 18),
            brush.Get());
    }
}

void CursorRenderer::DrawRipple(ID2D1DeviceContext* dc, const RippleState& ripple) {
    if (!ripple.active) return;

    D2D1_COLOR_F rippleColor = { 1.0f, 1.0f, 1.0f, ripple.opacity };
    ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT hr = dc->CreateSolidColorBrush(rippleColor, &brush);
    if (FAILED(hr)) return;

    D2D1_ELLIPSE ellipse = D2D1::Ellipse(
        D2D1::Point2F((FLOAT)ripple.center.x, (FLOAT)ripple.center.y),
        ripple.radius, ripple.radius);

    dc->DrawEllipse(ellipse, brush.Get(), ripple.strokeWidth);
}
