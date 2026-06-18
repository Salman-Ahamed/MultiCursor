#include "ClickEffects.h"
#include <algorithm>

void ClickEffects::Initialize(ID2D1DeviceContext* dc) {
    D2D1_COLOR_F white = { 1.0f, 1.0f, 1.0f, 0.6f };
    dc->CreateSolidColorBrush(white, &m_brush);
}

D2D1_COLOR_F ClickEffects::ColorForButton(MouseButton button) const {
    switch (button) {
    case MouseButton::Left:     return { 0.3f, 0.8f, 1.0f, 0.7f };
    case MouseButton::Right:    return { 1.0f, 0.3f, 0.3f, 0.7f };
    case MouseButton::Middle:   return { 0.3f, 1.0f, 0.3f, 0.7f };
    case MouseButton::XButton1: return { 1.0f, 0.8f, 0.3f, 0.7f };
    case MouseButton::XButton2: return { 0.8f, 0.4f, 1.0f, 0.7f };
    default:                    return { 1.0f, 1.0f, 1.0f, 0.6f };
    }
}

void ClickEffects::Trigger(POINT center, MouseButton button) {
    for (auto& e : m_effects) {
        if (!e.active) {
            e.center = center;
            e.radius = 0.0f;
            e.opacity = 1.0f;
            e.strokeWidth = 4.0f;
            e.elapsed = 0.0f;
            e.active = true;
            e.button = button;
            return;
        }
    }
    if (m_effects.size() < kMaxRipples) {
        Effect e;
        e.center = center;
        e.active = true;
        e.button = button;
        m_effects.push_back(e);
    }
}

void ClickEffects::Update(float dtMs) {
    for (auto& e : m_effects) {
        if (!e.active) continue;
        e.elapsed += dtMs;
        float t = e.elapsed / kRippleDurationMs;
        if (t >= 1.0f) {
            e.active = false;
            continue;
        }
        e.radius = t * kRippleMaxRadius;
        e.opacity = 1.0f - t;
        e.strokeWidth = 4.0f * (1.0f - t * 0.75f);
    }
}

void ClickEffects::Draw(ID2D1DeviceContext* dc) {
    if (!dc || !m_brush) return;
    for (auto& e : m_effects) {
        if (!e.active) continue;
        D2D1_COLOR_F color = ColorForButton(e.button);
        color.a *= e.opacity;
        m_brush->SetColor(color);
        D2D1_ELLIPSE re = D2D1::Ellipse(
            D2D1::Point2F((FLOAT)e.center.x, (FLOAT)e.center.y),
            e.radius, e.radius);
        dc->DrawEllipse(re, m_brush.Get(), e.strokeWidth);
    }
}

void ClickEffects::Clear() {
    m_effects.clear();
}
