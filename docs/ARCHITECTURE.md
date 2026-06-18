# MultiCursor Architecture

## Overview

MultiCursor is a Windows desktop application that allows multiple physical mice to control independent visible cursors via a transparent DirectComposition overlay. It is 100% user-mode Win32 with no kernel drivers, services, or DLL injection.

## Project Layout

```
src/
├── CMakeLists.txt          # Build system (CMake + MSBuild)
├── main.cpp                # Entry point, global ownership, shutdown orchestration
├── Core/                   # Cross-cutting domain logic
│   ├── AppStateMachine     # Finite state machine (Starting→Running→Suspended→ShuttingDown)
│   ├── ClickForwarder      # SendInput wrapper for click forwarding
│   ├── CursorManager       # Per-device cursor state (pos, buttons, ripples)
│   ├── DeviceManager       # Raw input device enumeration & hot-plug
│   ├── EventBus            # Thread-safe publish/subscribe template
│   ├── InputManager        # Per-device input state + ring buffer
│   ├── Logger              # Structured logging (file + debug output)
│   ├── SettingsManager     # JSON-based persistent settings
│   └── Types.h             # Shared type definitions
├── Platform/               # Win32 platform glue
│   ├── RawInputHandler     # Raw Input API registration + WM_INPUT dispatch
│   └── WindowManager       # Overlay window + DComp surface + session/power events
├── Rendering/              # Direct2D/DirectComposition rendering pipeline
│   ├── Direct2DRenderer    # Low-level D2D render operations
│   └── OverlayRenderer     # Composition surface management + render loop
└── UI/                     # Win32 UI windows
    ├── MainWindow          # Main control window (device list, tray icon)
    └── SettingsWindow      # Settings dialog (overlay toggle, cursor size, opacity)
```

## Thread Model

```
┌─────────────────────────────────────────────────────────────────┐
│                        Main Thread                              │
│  main.cpp → Init → Run loop → Shutdown                         │
│  - Creates all globals                                          │
│  - Owns AppStateMachine, DeviceManager, CursorManager, etc.    │
│  - Hosts MainWindow + SettingsWindow (UI message pump)          │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       │ PostMessage(WM_INPUT)
                       │
┌──────────────────────▼──────────────────────────────────────────┐
│                      Input Thread                               │
│  RawInputHandler message pump                                   │
│  - Registers raw input devices (RIDEV_INPUTSINK|0x8000)        │
│  - Drains WM_INPUT via GetRawInputBuffer                        │
│  - Publishes input events to InputManager ring buffer           │
│  - Self-destructs on shutdown (PostMessage wakeup)              │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       │ Shared state (mutex-protected)
                       │
┌──────────────────────▼──────────────────────────────────────────┐
│                     Render Thread                                │
│  OverlayRenderer::RenderLoop                                    │
│  - DComp composition clock / QPC frame pacing                   │
│  - Reads cursor snapshots via CursorManager::SnapshotCursors    │
│  - Draws via Direct2DRenderer on IDCompositionSurface           │
│  - Handles device lost → recovery chain                         │
│  - Forwards clicks via ClickForwarder                           │
└─────────────────────────────────────────────────────────────────┘
```

## Data Flow

```
Physical Mouse
     │
     ▼
Raw Input API (GetRawInputBuffer)
     │
     ▼
InputManager ring buffer (lock-free, 2048 entries)
     │
     ├──▶ CursorManager::UpdateButton / UpdatePosition
     │         │
     │         ▼
     │    SnapshotCursors → render thread reads atomically
     │
     └──▶ OverlayRenderer::LateInputSample (per-frame)
                │
                ├──▶ ForwardButtonDown / ForwardButtonUp (SendInput)
                │
                ▼
         Click forwarded to target window
```

## State Machine

```
Starting ──▶ Running ◀──▶ Suspended
  │              │              │
  │              ├──▶ DeviceLost ──▶ Recovering ──▶ Running
  │              │
  └──▶ ShuttingDown
```

Transitions:
- `Starting→Running`: Enumeration succeeds, overlay window shown
- `Running→Suspended`: Session lock (WTS), overlay hidden
- `Suspended→Running`: Session unlock, overlay shown (if enabled)
- `Running→ShuttingDown`: App exit
- `Running→DeviceLost`: DComp surface lost → `Recovering`
- `Recovering→Running`: Re-created device + re-registered cursors

## Key Technical Decisions

- **Raw input bypass flag**: `0x8000` bypasses Win11 KB5028185 background throttle. Combined: `RIDEV_INPUTSINK | RIDEV_DEVNOTIFY | 0x8000` = `0x2900`
- **DComp surface (not swap chain)**: `IDCompositionSurface::BeginDraw/EndDraw` works with any D3D device including WARP; `CreateSwapChainForComposition` does not support software drivers
- **SendInput guard**: All synthetic input tagged with `dwExtraInfo = 0x4D430001`; input thread skips processing its own clicks
- **No `RIDEV_NOLEGACY`**: Breaks window chrome; instead listen as sink and forward clicks explicitly
- **ComCtrl6 manifest**: Embedded via `#pragma comment(linker, "/manifestdependency:...")` — no external `.manifest` file
