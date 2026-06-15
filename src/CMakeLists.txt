set(SOURCES
    main.cpp
    Core/Logger.cpp
    Core/SettingsManager.cpp
    Core/AppStateMachine.cpp
    Core/DeviceManager.cpp
    Core/InputManager.cpp
    Core/CursorManager.cpp
    Platform/RawInputHandler.cpp
    Platform/WindowManager.cpp
    Rendering/Direct2DRenderer.cpp
    Rendering/OverlayRenderer.cpp
    Rendering/CursorRenderer.cpp
    UI/MainWindow.cpp
    UI/SettingsWindow.cpp
)

add_executable(MultiCursor WIN32 ${SOURCES})

target_include_directories(MultiCursor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(MultiCursor PRIVATE
    dcomp.lib
    d2d1.lib
    dwrite.lib
    dxgi.lib
    d3d11.lib
    hid.lib
    user32.lib
    shell32.lib
    advapi32.lib
    ole32.lib
    oleaut32.lib
    shlwapi.lib
    comctl32.lib
    comdlg32.lib
)

set_target_properties(MultiCursor PROPERTIES
    WIN32_EXECUTABLE TRUE
)
