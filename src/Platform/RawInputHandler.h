#pragma once

#include "Core/Types.h"
#include "Core/InputManager.h"
#include "Core/DeviceManager.h"
#include <functional>

class RawInputHandler {
public:
    using InputCallback = std::function<void(HANDLE, const RAWINPUT&)>;

    RawInputHandler(InputManager& inputMgr, DeviceManager& devMgr);
    ~RawInputHandler();

    bool Initialize();
    void Shutdown();
    bool ProcessMessage(MSG& msg);

    HWND WindowHandle() const { return m_hwnd; }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void OnInput(WPARAM wParam, LPARAM lParam);
    void DrainRawInputBuffer();
    void OnDeviceChange(WPARAM wParam, LPARAM lParam);

    InputManager& m_inputMgr;
    DeviceManager& m_devMgr;
    HWND m_hwnd = nullptr;
    std::vector<uint8_t> m_buffer;

    static RawInputHandler* s_instance;
};
