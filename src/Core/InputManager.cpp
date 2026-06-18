#include "InputManager.h"
#include "Logger.h"
#include <cstring>

InputManager::InputManager() {
    m_deviceHandles.fill(nullptr);
    m_states.fill(MouseState{});
}

void InputManager::ProcessInput(HANDLE hDevice, const RAWINPUT& raw) {
    UINT idx = LookupIndex(hDevice);
    if (idx == UINT_MAX) return;
    auto& state = m_states[idx];

    auto& mouse = raw.data.mouse;

    if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
        if (mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) {
            state.lx = (mouse.lLastX * GetSystemMetrics(SM_CXVIRTUALSCREEN)) / 65536;
            state.ly = (mouse.lLastY * GetSystemMetrics(SM_CYVIRTUALSCREEN)) / 65536;
        } else {
            state.lx = (mouse.lLastX * GetSystemMetrics(SM_CXSCREEN)) / 65536;
            state.ly = (mouse.lLastY * GetSystemMetrics(SM_CYSCREEN)) / 65536;
        }
        state.absolute = true;
    } else {
        state.dx = mouse.lLastX;
        state.dy = mouse.lLastY;
        state.absolute = false;
    }

    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) state.buttons[0] = true;
    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP)   state.buttons[0] = false;
    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) state.buttons[1] = true;
    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP)   state.buttons[1] = false;
    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) state.buttons[2] = true;
    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP)   state.buttons[2] = false;
    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) state.buttons[3] = true;
    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP)   state.buttons[3] = false;
    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) state.buttons[4] = true;
    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP)   state.buttons[4] = false;

    state.buttonFlags = mouse.usButtonFlags;

    if (mouse.usButtonFlags & RI_MOUSE_WHEEL) {
        state.wheelDelta = (SHORT)mouse.usButtonData;
    }
    if (mouse.usButtonFlags & RI_MOUSE_HWHEEL) {
        state.wheelHDelta = (SHORT)mouse.usButtonData;
    }
}

UINT InputManager::LookupIndex(HANDLE hDevice) {
    for (UINT i = 0; i < kMaxDevices; i++) {
        if (m_deviceHandles[i] == hDevice)
            return i;
        if (m_deviceHandles[i] == nullptr) {
            m_deviceHandles[i] = hDevice;
            m_states[i] = MouseState{};
            return i;
        }
    }
    return UINT_MAX;
}

void InputManager::Flush() {
    for (auto& state : m_states) {
        state.dx = 0;
        state.dy = 0;
        state.wheelDelta = 0;
        state.wheelHDelta = 0;
        state.buttonFlags = 0;
    }
}

void InputManager::PublishToRingBuffer() {
    size_t head = m_head.load(std::memory_order_acquire);
    for (UINT i = 0; i < kMaxDevices; i++) {
        if (m_deviceHandles[i] == nullptr) continue;

        auto& state = m_states[i];
        bool hasData = (state.dx != 0 || state.dy != 0 || state.buttonFlags != 0 ||
                        state.wheelDelta != 0 || state.wheelHDelta != 0 || state.absolute);

        if (!hasData) continue;

        size_t next = (head + 1) & kMask;
        if (next == m_tail.load(std::memory_order_acquire)) {
            LOG_WARN(L"Ring buffer full, dropping input frame");
            break;
        }

        m_ringBuffer[head].hDevice = m_deviceHandles[i];
        m_ringBuffer[head].state = state;
        m_head.store(next, std::memory_order_release);
    }

    Flush();
}

bool InputManager::TryConsume(RawInputFrame& frame) {
    size_t tail = m_tail.load(std::memory_order_acquire);
    size_t head = m_head.load(std::memory_order_acquire);
    if (tail == head) return false;

    frame = m_ringBuffer[tail];
    m_tail.store((tail + 1) & kMask, std::memory_order_release);
    return true;
}

void InputManager::ResetDevice(HANDLE hDevice) {
    for (UINT i = 0; i < kMaxDevices; i++) {
        if (m_deviceHandles[i] == hDevice) {
            m_deviceHandles[i] = nullptr;
            m_states[i] = MouseState{};
            break;
        }
    }
}
