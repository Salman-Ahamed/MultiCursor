#include "Core/Logger.h"
#include "Core/SettingsManager.h"
#include "Core/EventBus.h"
#include "Core/AppStateMachine.h"
#include "Core/DeviceManager.h"
#include "Core/InputManager.h"
#include "Core/CursorManager.h"
#include "Platform/RawInputHandler.h"
#include "Platform/WindowManager.h"
#include "Rendering/Direct2DRenderer.h"
#include "Rendering/OverlayRenderer.h"
#include "UI/MainWindow.h"
#include "UI/SettingsWindow.h"

#include <thread>
#include <shellapi.h>
#include <shlobj.h>
#include <dbghelp.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dbghelp.lib")

namespace {
    HINSTANCE g_hInstance = nullptr;
    SettingsManager g_settings;
    DeviceEventBus g_deviceBus;
    ErrorEventBus g_errorBus;
    AppStateMachine g_stateMachine;
    std::unique_ptr<DeviceManager> g_deviceMgr;
    std::unique_ptr<InputManager> g_inputMgr;
    std::unique_ptr<CursorManager> g_cursorMgr;
    std::unique_ptr<RawInputHandler> g_rawInput;
    std::unique_ptr<WindowManager> g_windowMgr;
    std::unique_ptr<Direct2DRenderer> g_renderer;
    std::unique_ptr<OverlayRenderer> g_overlay;
    std::unique_ptr<MainWindow> g_mainWindow;
    std::unique_ptr<SettingsWindow> g_settingsWindow;
    std::thread g_inputThread;
    NOTIFYICONDATAW g_trayIcon = {};
    bool g_running = true;
    std::wstring g_logDir;
}

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* exc) {
    LOG_ERROR(L"Unhandled exception: 0x%08X at 0x%p",
              exc->ExceptionRecord->ExceptionCode,
              exc->ExceptionRecord->ExceptionAddress);

    HANDLE hFile = CreateFileW(
        (g_logDir + L"\\crash.dmp").c_str(),
        GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei = { GetCurrentThreadId(), exc, FALSE };
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                          hFile, MiniDumpNormal, &mei, nullptr, nullptr);
        CloseHandle(hFile);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

void InputThreadProc() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (!g_rawInput->Initialize()) {
        LOG_ERROR(L"Failed to initialize RawInputHandler");
        g_stateMachine.TransitionTo(AppState::ShuttingDown);
        return;
    }

    g_deviceMgr->Enumerate();

    // Add cursors for all detected mouse devices
    for (auto& dev : g_deviceMgr->Devices()) {
        if (dev.isMouse) {
            auto label = dev.name;
            auto hashPos = label.find_last_of(L'#');
            if (hashPos != std::wstring::npos) {
                auto colPos = label.rfind(L'#', hashPos - 1);
                if (colPos != std::wstring::npos && colPos + 1 < label.size()) {
                    label = label.substr(colPos + 1, hashPos - colPos - 1);
                } else {
                    label = label.substr(0, hashPos);
                }
            }
            g_cursorMgr->AddCursor(dev.hDevice, label);
        }
    }

    g_stateMachine.TransitionTo(AppState::Running);

    MSG msg;
    while (g_running && GetMessage(&msg, nullptr, 0, 0)) {
        if (!g_rawInput->ProcessMessage(msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        static ULONGLONG lastPublish = 0;
        ULONGLONG now = GetTickCount64();
        if (now - lastPublish >= 1) {
            g_inputMgr->PublishToRingBuffer();
            lastPublish = now;
        }
    }

    g_rawInput->Shutdown();
    CoUninitialize();
}

void InitTrayIcon(HWND hwnd) {
    g_trayIcon.cbSize = sizeof(NOTIFYICONDATAW);
    g_trayIcon.hWnd = hwnd;
    g_trayIcon.uID = 1;
    g_trayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_trayIcon.uCallbackMessage = WM_APP + 1;
    g_trayIcon.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_trayIcon.szTip, L"MultiCursor");
    Shell_NotifyIconW(NIM_ADD, &g_trayIcon);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInstance = hInstance;

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetUnhandledExceptionFilter(CrashHandler);

    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    std::wstring exePath(modulePath);
    g_logDir = exePath.substr(0, exePath.find_last_of(L'\\')) + L"\\logs";
    CreateDirectoryW(g_logDir.c_str(), nullptr);

    Logger::Instance().Init(g_logDir);
    Logger::Instance().SetLevel(Severity::Debug);
    LOG_INFO(L"MultiCursor starting...");

    // Load settings
    wchar_t appData[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appData);
    std::wstring configPath = std::wstring(appData) + L"\\MultiCursor\\config.json";
    CreateDirectoryW((std::wstring(appData) + L"\\MultiCursor").c_str(), nullptr);
    g_settings.Load(configPath);

    // Initialize UI
    g_mainWindow = std::make_unique<MainWindow>();
    if (!g_mainWindow->Initialize(hInstance)) {
        LOG_ERROR(L"Failed to initialize MainWindow");
        return 1;
    }

    g_settingsWindow = std::make_unique<SettingsWindow>();
    g_settingsWindow->Initialize(hInstance, g_mainWindow->Handle());

    // Initialize overlay window
    g_windowMgr = std::make_unique<WindowManager>();
    if (!g_windowMgr->Initialize()) {
        LOG_ERROR(L"Failed to initialize WindowManager");
        return 1;
    }

    // Initialize renderer
    g_renderer = std::make_unique<Direct2DRenderer>();
    if (!g_renderer->Initialize(g_windowMgr->Handle())) {
        LOG_ERROR(L"Failed to initialize Direct2DRenderer");
        return 1;
    }

    // Create shared core objects before starting threads
    g_deviceMgr = std::make_unique<DeviceManager>(g_deviceBus);
    g_inputMgr = std::make_unique<InputManager>();
    g_cursorMgr = std::make_unique<CursorManager>(g_deviceBus);
    g_rawInput = std::make_unique<RawInputHandler>(*g_inputMgr, *g_deviceMgr);

    // Create overlay renderer
    g_overlay = std::make_unique<OverlayRenderer>(*g_renderer, *g_cursorMgr, *g_inputMgr, *g_deviceMgr);

    // Register app restart
    RegisterApplicationRestart(L"-restart", RESTART_NO_CRASH | RESTART_NO_HANG);

    // System tray icon
    InitTrayIcon(g_mainWindow->Handle());

    // Register hotkey
    RegisterHotKey(g_mainWindow->Handle(), 1, MOD_CONTROL | MOD_ALT, 'M');

    // Start input thread
    g_stateMachine.TransitionTo(AppState::Starting);
    g_inputThread = std::thread(InputThreadProc);

    // Start overlay rendering
    g_overlay->Start();

    // Show main window
    g_mainWindow->Show(nCmdShow);

    LOG_INFO(L"MultiCursor initialized, entering message loop");

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_APP + 1) {
            switch (msg.lParam) {
            case WM_LBUTTONDBLCLK:
                ShowWindow(g_mainWindow->Handle(), SW_SHOW);
                SetForegroundWindow(g_mainWindow->Handle());
                break;
            }
        }

        if (msg.message == WM_HOTKEY && msg.wParam == 1) {
            if (IsWindowVisible(g_mainWindow->Handle())) {
                ShowWindow(g_mainWindow->Handle(), SW_HIDE);
            } else {
                ShowWindow(g_mainWindow->Handle(), SW_SHOW);
                SetForegroundWindow(g_mainWindow->Handle());
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Shutdown
    LOG_INFO(L"MultiCursor shutting down");
    g_running = false;
    g_stateMachine.TransitionTo(AppState::ShuttingDown);

    g_overlay->Stop();
    if (g_inputThread.joinable()) g_inputThread.join();

    Shell_NotifyIconW(NIM_DELETE, &g_trayIcon);
    UnregisterHotKey(g_mainWindow->Handle(), 1);

    LOG_INFO(L"MultiCursor exited");
    return 0;
}
