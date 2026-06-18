# Setup Guide

## Prerequisites

- Windows 10 1809+ or Windows 11
- Visual Studio 2022 with the following workloads:
  - Desktop development with C++
  - Windows 10/11 SDK (10.0.20348.0 or later)
- CMake 3.20+

## Building

```powershell
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Build
cmake --build build --config RelWithDebInfo

# Output
#   build/src/RelWithDebInfo/MultiCursor.exe
#   build/Tests/RelWithDebInfo/MultiCursorTests.exe
```

## Running

```powershell
# Run the application
.\build\src\RelWithDebInfo\MultiCursor.exe

# Run tests
.\build\Tests\RelWithDebInfo\MultiCursorTests.exe
```

**Note**: `MultiCursor.exe` must be run from an elevated (Administrator) or UIAccess-signed context for click forwarding (`SendInput`) to work across integrity levels. Without elevation or signing, clicks can only be forwarded to same-IL windows.

## Code Signing (for UIAccess)

The UIAccess manifest requires a digital signature. A self-signed development certificate is provided:

```powershell
# Build with signing enabled (uses resources/MultiCursorDev.pfx)
cmake -S . -B build -DENABLE_SIGNING=ON
cmake --build build --config RelWithDebInfo
```

**Important**: The self-signed certificate is for **development only**. For production, replace with a real CA-signed certificate:
1. Purchase a code signing certificate from a trusted CA
2. Update `PFX_PATH` and `PFX_PASSWORD` in `src/CMakeLists.txt` 
3. Rebuild with `-DENABLE_SIGNING=ON`

Without signing, the application runs normally but `SendInput` cannot forward clicks to elevated (Administrator) windows.

## Build Presets

The project includes `CMakePresets.json` with a `default` preset. Use it with:

```powershell
cmake --preset default
cmake --build build
```

## Debugging

Logs are written to:
- Output debug string (visible in DebugView or VS Output window)
- File: `%LOCALAPPDATA%\MultiCursor\logs\MultiCursor.log`

## Removing

Delete the build directory and the repository. No system-wide changes are made (no registry, no services, no drivers).

```
rm -rf build
```
