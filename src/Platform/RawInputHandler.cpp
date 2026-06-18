#include "RawInputHandler.h"
#include "Core/Logger.h"

RawInputHandler* RawInputHandler::s_instance = nullptr;

RawInputHandler::RawInputHandler(InputManager& inputMgr, DeviceManager& devMgr)
    : m_inputMgr(inputMgr), m_devMgr(devMgr) {
    s_instance = this;
}

RawInputHandler::~RawInputHandler() {
    Shutdown();
    if (s_instance == this) s_instance = nullptr;
}

bool RawInputHandler::Initialize() {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"MultiCursorRawInputWindow";

    if (!RegisterClassExW(&wc)) {
        LOG_ERROR(L"Failed to register raw input window class");
        return false;
    }

    m_hwnd = CreateWindowExW(0, L"MultiCursorRawInputWindow", L"RawInput",
                             0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    if (!m_hwnd) {
        LOG_ERROR(L"Failed to create raw input message window");
        return false;
    }

    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY | 0x8000;
    rid.hwndTarget = m_hwnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        LOG_ERROR(L"RegisterRawInputDevices failed: %d", GetLastError());
        return false;
    }

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

    m_buffer.resize(64 * sizeof(RAWINPUT) + sizeof(RAWINPUTHEADER) + 16);

    LOG_INFO(L"RawInputHandler initialized");
    return true;
}

void RawInputHandler::Shutdown() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

bool RawInputHandler::ProcessMessage(MSG& msg) {
    if (msg.hwnd != m_hwnd) return false;

    switch (msg.message) {
    case WM_INPUT:
        OnInput(msg.wParam, msg.lParam);
        return true;
    case WM_INPUT_DEVICE_CHANGE:
        OnDeviceChange(msg.wParam, msg.lParam);
        return true;
    }
    return false;
}

LRESULT CALLBACK RawInputHandler::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    auto* self = (RawInputHandler*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (self) {
        switch (msg) {
        case WM_INPUT:
            self->OnInput(wParam, lParam);
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        case WM_INPUT_DEVICE_CHANGE:
            self->OnDeviceChange(wParam, lParam);
            return 0;
        }
    }

    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void RawInputHandler::OnInput(WPARAM wParam, LPARAM lParam) {
    UINT size = (UINT)m_buffer.size();
    UINT result = GetRawInputData((HRAWINPUT)lParam, RID_INPUT,
                                   m_buffer.data(), &size, sizeof(RAWINPUTHEADER));

    if (result == (UINT)-1) {
        LOG_WARN(L"GetRawInputData failed: %d", GetLastError());
        return;
    }

    RAWINPUT* raw = (RAWINPUT*)m_buffer.data();
    if (raw->header.dwType == RIM_TYPEMOUSE) {
                if (raw->data.mouse.ulExtraInformation == kSendInputMagic) return;
        m_inputMgr.ProcessInput(raw->header.hDevice, *raw);
    }

    DrainRawInputBuffer();
}

void RawInputHandler::DrainRawInputBuffer() {
    for (;;) {
        UINT size = (UINT)m_buffer.size();
        UINT count = GetRawInputBuffer((RAWINPUT*)m_buffer.data(), &size, sizeof(RAWINPUTHEADER));

        if (count == 0 || count == (UINT)-1) break;

        RAWINPUT* raw = (RAWINPUT*)m_buffer.data();
        for (UINT i = 0; i < count; i++) {
            if (raw->header.dwType == RIM_TYPEMOUSE) {
                if (raw->data.mouse.ulExtraInformation == kSendInputMagic) continue;
                m_inputMgr.ProcessInput(raw->header.hDevice, *raw);
            }
            raw = (RAWINPUT*)((uint8_t*)raw + raw->header.dwSize);
        }
    }
}

void RawInputHandler::OnDeviceChange(WPARAM wParam, LPARAM lParam) {
    if (wParam == GIDC_ARRIVAL) {
        LOG_INFO(L"Raw input device arrived");
        m_devMgr.OnDeviceArrival();
    } else if (wParam == GIDC_REMOVAL) {
        LOG_INFO(L"Raw input device removed");
        m_devMgr.OnDeviceRemoval();
    }
}
