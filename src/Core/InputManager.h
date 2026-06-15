#pragma once

#include "Types.h"
#include <array>
#include <atomic>
#include <cstddef>

class InputManager {
public:
    InputManager();

    void ProcessInput(HANDLE hDevice, const RAWINPUT& raw);
    void Flush();
    void PublishToRingBuffer();

    UINT LookupIndex(HANDLE hDevice);
    void ResetDevice(HANDLE hDevice);

    const MouseState& GetState(UINT idx) const { return m_states[idx]; }

    // Ring buffer consumer (called from render thread)
    bool TryConsume(RawInputFrame& frame);

private:
    std::array<MouseState, kMaxDevices> m_states;
    std::array<HANDLE, kMaxDevices> m_deviceHandles = {};
    UINT m_nextIndex = 0;

    // Lock-free SPSC ring buffer
    std::array<RawInputFrame, kRingBufferSize> m_ringBuffer;
    std::atomic<size_t> m_head{ 0 };
    std::atomic<size_t> m_tail{ 0 };
    static constexpr size_t kMask = kRingBufferSize - 1;
};
