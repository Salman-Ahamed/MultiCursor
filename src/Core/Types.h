#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <dcomp.h>
#include <dxgi1_2.h>
#include <d3d11.h>

#include <cstdint>
#include <string>
#include <array>
#include <functional>
#include <memory>

constexpr UINT kMaxDevices = 16;
constexpr UINT kRingBufferSize = 2048;
constexpr UINT kSendInputMagic = 0x4D430001;
constexpr UINT kMaxRipples = 64;
constexpr float kCursorRadius = 12.0f;
constexpr float kRippleMaxRadius = 40.0f;
constexpr float kRippleDurationMs = 300.0f;

enum class AppState {
    Starting,
    Running,
    Suspended,
    DeviceLost,
    Recovering,
    ShuttingDown
};

enum class DeviceType {
    Mouse,
    Touchpad,
    TouchScreen,
    Pen,
    Keyboard,
    Other
};

enum class MouseButton {
    Left = 0,
    Right,
    Middle,
    XButton1,
    XButton2
};

enum class Severity {
    Debug,
    Info,
    Warn,
    Error
};

struct MouseState {
    LONG dx = 0, dy = 0;
    LONG lx = 0, ly = 0;
    bool absolute = false;
    bool buttons[5] = {};
    USHORT buttonFlags = 0;
    SHORT wheelDelta = 0;
    SHORT wheelHDelta = 0;
    ULONG timestamp = 0;
};

struct RawInputFrame {
    HANDLE hDevice = nullptr;
    MouseState state;
};

struct CursorState {
    POINT pos = {};
    DWORD color = 0xFFFFFFFF;
    std::wstring label;
    bool buttons[5] = {};
    bool visible = true;
    bool dirty = true;
};

struct RippleState {
    POINT center = {};
    float radius = 0.0f;
    float opacity = 1.0f;
    float strokeWidth = 4.0f;
    float elapsed = 0.0f;
    bool active = false;
};

struct DeviceInfo {
    HANDLE hDevice = nullptr;
    std::wstring name;
    std::wstring path;
    DeviceType type = DeviceType::Other;
    bool isMouse = false;
};

struct DeviceEvent {
    enum Type { Added, Removed, Changed };
    Type type;
    DeviceInfo device;
};

struct CursorEvent {
    enum Type { Moved, ButtonDown, ButtonUp, ScrollChanged, ColorChanged, LabelChanged, VisibilityChanged };
    Type type;
    UINT cursorId;
    int x, y;
    int button;
    std::wstring label;
    DWORD color;
    bool visible;
};

struct ErrorEvent {
    enum Source { Input, Render, Device, Config };
    Source source;
    HRESULT hr;
    std::wstring message;
};

struct CursorProfile {
    DWORD color = 0xFFE6194B;
    std::wstring label;
    float cursorSize = 12.0f;
    float opacity = 0.8f;
    float speedMultiplier = 1.0f;
    bool swapButtons = false;
    bool reverseScroll = false;
    int scrollLines = 3;
    bool snapToDefault = false;
    bool enhancePrecision = false;
    int doubleClickSpeed = 500;
    bool clickLock = false;
    std::wstring monitorBinding;
};

struct AppSettings {
    bool overlayEnabled = true;
    bool showLabels = true;
    float globalOpacity = 0.8f;
    int logLevel = 1;
    bool startWithWindows = false;
    bool minimizeToTray = true;
};

constexpr DWORD kCursorColors[] = {
    0xFFE6194B, 0xFF3CB44B, 0xFF4363D8, 0xFFF58231,
    0xFF911EB4, 0xFF42D4F4, 0xFFF032E6, 0xFFBFEF45,
    0xFFFABED4, 0xFF469990, 0xFFDCBEFF, 0xFF9A6324
};
constexpr UINT kCursorColorCount = 12;
