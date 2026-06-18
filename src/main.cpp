#include "Core/Logger.h"
#include "Core/SettingsManager.h"
#include "Core/EventBus.h"
#include "Core/AppStateMachine.h"
#include "Core/DeviceManager.h"
#include "Core/InputManager.h"
#include "Core/CursorManager.h"
#include "Core/ProfileManager.h"
#include "Platform/RawInputHandler.h"
#include "Platform/WindowManager.h"
#include "Platform/DeviceEnumerator.h"
#include "Platform/WindowsSettings.h"
#include "Platform/MonitorManager.h"
#include "Rendering/Direct2DRenderer.h"
#include "Rendering/OverlayRenderer.h"
#include "UI/MainWindow.h"
#include "UI/SettingsWindow.h"

#include <thread>
#include <atomic>
#include <shellapi.h>
#include <shlobj.h>
#include <dbghelp.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {
    HINSTANCE g_hInstance = nullptr;
    DeviceEventBus g_deviceBus;
    CursorEventBus g_cursorBus;
    KeyboardEventBus g_keyboardBus;
    ErrorEventBus g_errorBus;
    SettingsEventBus g_settingsBus;
    StateEventBus g_stateBus;
    SettingsManager g_settings{g_settingsBus};
    AppStateMachine g_stateMachine{g_stateBus};
    std::unique_ptr<DeviceManager> g_deviceMgr;
    std::unique_ptr<InputManager> g_inputMgr;
    std::unique_ptr<ProfileManager> g_profileMgr;
    std::unique_ptr<CursorManager> g_cursorMgr;
    std::unique_ptr<RawInputHandler> g_rawInput;
    std::unique_ptr<WindowManager> g_windowMgr;
    std::unique_ptr<Direct2DRenderer> g_renderer;
    std::unique_ptr<OverlayRenderer> g_overlay;
    std::unique_ptr<MainWindow> g_mainWindow;
    std::unique_ptr<SettingsWindow> g_settingsWindow;
    std::thread g_inputThread;
    std::atomic<bool> g_running{ true };
    std::wstring g_logDir;
    std::wstring g_profileDir;

    std::wstring ExtractLabel(const DeviceInfo& dev) {
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
        return label;
    }
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
    SetThreadDescription(GetCurrentThread(), L"MultiCursor Input");
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (!g_rawInput->Initialize()) {
        LOG_ERROR(L"Failed to initialize RawInputHandler");
        g_stateMachine.TransitionTo(AppState::ShuttingDown);
        return;
    }

    g_deviceMgr->Enumerate();

    // Refresh main window device list on UI thread
    PostMessageW(g_mainWindow->Handle(), WM_APP + 2, 0, 0);

    // Subscribe to device events for hot-plug updates
    auto deviceSub = g_deviceBus.Subscribe([&](const DeviceEvent& e) {
        // Notify main window (on UI thread via PostMessage)
        PostMessageW(g_mainWindow->Handle(), WM_APP + 2, 0, 0);

        if (e.type == DeviceEvent::Added) {
            g_mainWindow->Tray().ShowBalloon(L"Device Connected", e.device.name.c_str(), NIIF_INFO);
        } else if (e.type == DeviceEvent::Removed) {
            g_mainWindow->Tray().ShowBalloon(L"Device Disconnected", e.device.name.c_str(), NIIF_WARNING);
        }

        // Add cursors for any new devices
        for (auto& dev : g_deviceMgr->Devices()) {
            if (!dev.isMouse) continue;
            UINT existingIdx = g_cursorMgr->LookupCursor(dev.hDevice);
            if (existingIdx == (UINT)-1) {
                auto label = ExtractLabel(dev);
                g_cursorMgr->AddCursor(dev.hDevice, dev.path, label);
            }
        }
    });

    // Add cursors for all detected mouse devices
    for (auto& dev : g_deviceMgr->Devices()) {
        if (dev.isMouse) {
            auto label = ExtractLabel(dev);
            g_cursorMgr->AddCursor(dev.hDevice, dev.path, label);
        }
    }

    g_stateMachine.TransitionTo(AppState::Running);

    MSG msg = {};
    while (g_running.load()) {
        BOOL ret = GetMessage(&msg, nullptr, 0, 0);
        if (ret <= 0) break;
        if (!g_running.load()) break;
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInstance = hInstance;

    // Check for restart flag
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool isRestart = false;
    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"-restart") == 0) {
            isRestart = true;
            break;
        }
    }
    LocalFree(argv);

    if (isRestart) {
        LOG_INFO(L"Restarted after previous crash");
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetUnhandledExceptionFilter(CrashHandler);

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

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
    g_settingsWindow->Initialize(hInstance, g_mainWindow->Handle(), g_settings);
    g_mainWindow->SetSettingsWindow(g_settingsWindow.get());

    // Initialize overlay window
    g_windowMgr = std::make_unique<WindowManager>(g_stateMachine);
    g_windowMgr->SetDisplayChangeCallback([](int w, int h) {
        if (g_renderer) g_renderer->ResizeSurface(w, h);
    });
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

    // Create profile directory
    g_profileDir = std::wstring(appData) + L"\\MultiCursor\\profiles";
    CreateDirectoryW(g_profileDir.c_str(), nullptr);

    // Create shared core objects before starting threads
    g_deviceMgr = std::make_unique<DeviceManager>(g_deviceBus);
    g_inputMgr = std::make_unique<InputManager>();
    g_profileMgr = std::make_unique<ProfileManager>(g_profileDir);
    g_cursorMgr = std::make_unique<CursorManager>(g_deviceBus, *g_profileMgr);
    g_rawInput = std::make_unique<RawInputHandler>(*g_inputMgr, *g_deviceMgr, g_keyboardBus);

    // Wire device manager and cursor manager to main window
    g_mainWindow->SetDeviceManager(g_deviceMgr.get());
    g_mainWindow->SetCursorManager(g_cursorMgr.get());
    if (g_settingsWindow) g_settingsWindow->SetCursorManager(g_cursorMgr.get());

    // Create overlay renderer
    g_overlay = std::make_unique<OverlayRenderer>(*g_renderer, *g_cursorMgr, *g_inputMgr,
                                                   g_settings, g_stateMachine);

    // Wire keyboard event bus
    g_keyboardBus.Subscribe([](const KeyboardEvent& e) {
        LOG_DEBUG(L"Keyboard: VKey=0x%04X MakeCode=0x%04X Flags=0x%04X", e.virtualKey, e.makeCode, e.flags);
    });

    // Wire error event bus
    g_errorBus.Subscribe([](const ErrorEvent& e) {
        LOG_ERROR(L"[ErrorEvent] source=%d hr=0x%08X msg=%s",
                  (int)e.source, e.hr, e.message.c_str());
    });
    RegisterApplicationRestart(L"-restart", RESTART_NO_CRASH | RESTART_NO_HANG);

    // Wire state machine transitions
    g_stateMachine.OnTransition([](AppState from, AppState to) {
        LOG_INFO(L"State: %d -> %d", (int)from, (int)to);
        if (from == AppState::DeviceLost && to == AppState::Recovering) {
            LOG_INFO(L"Attempting device recovery...");
            // Re-enumerate devices and re-add cursors (Recreate already done by render loop)
            if (g_deviceMgr) g_deviceMgr->Enumerate();
            if (g_cursorMgr && g_deviceMgr) {
                for (auto& dev : g_deviceMgr->Devices()) {
                    if (dev.isMouse) {
                        UINT existingIdx = g_cursorMgr->LookupCursor(dev.hDevice);
                        if (existingIdx == (UINT)-1) {
                            g_cursorMgr->AddCursor(dev.hDevice, dev.path, dev.name);
                        }
                    }
                }
            }
            LOG_INFO(L"Device recovery successful");
            g_stateMachine.TransitionTo(AppState::Running);
        }
        if (to == AppState::Suspended) {
            if (g_windowMgr) g_windowMgr->Hide();
            if (g_overlay) g_overlay->Suspend();
        }
        if (to == AppState::Running && from == AppState::Suspended) {
            if (g_overlay) g_overlay->Resume();
            if (g_windowMgr && g_windowMgr->IsOverlayEnabled()) g_windowMgr->Show();
        }
    });

    // System tray icon
    g_mainWindow->Tray().Initialize(hInstance, g_mainWindow->Handle());
    g_mainWindow->Tray().ShowBalloon(L"MultiCursor", L"MultiCursor is running in the background");

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
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
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
    g_running.store(false);

    // Wake input thread from GetMessage so it can exit & self-cleanup
    if (g_rawInput) {
        HWND hwnd = g_rawInput->WindowHandle();
        if (hwnd) PostMessageW(hwnd, WM_NULL, 0, 0);
    }

    g_stateMachine.TransitionTo(AppState::ShuttingDown);
    g_overlay->Stop();
    if (g_inputThread.joinable()) g_inputThread.join();

    g_mainWindow->Tray().Remove();
    UnregisterHotKey(g_mainWindow->Handle(), 1);

    LOG_INFO(L"MultiCursor exited");
    return 0;
}
