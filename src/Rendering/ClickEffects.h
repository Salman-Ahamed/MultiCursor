#pragma once

#include "Core/Types.h"
#include <vector>
#include <wrl/client.h>
#include <d2d1_1.h>

using Microsoft::WRL::ComPtr;

class ClickEffects {
public:
    ClickEffects() = default;
    ~ClickEffects() = default;

    void Initialize(ID2D1DeviceContext* dc);
    void Trigger(POINT center, MouseButton button);
    void Update(float dtMs);
    void Draw(ID2D1DeviceContext* dc);
    void Clear();

private:
    D2D1_COLOR_F ColorForButton(MouseButton button) const;

    struct Effect {
        POINT center = {};
        float radius = 0.0f;
        float opacity = 1.0f;
        float strokeWidth = 4.0f;
        float elapsed = 0.0f;
        bool active = false;
        MouseButton button = MouseButton::Left;
    };

    std::vector<Effect> m_effects;
    ComPtr<ID2D1SolidColorBrush> m_brush;
};
