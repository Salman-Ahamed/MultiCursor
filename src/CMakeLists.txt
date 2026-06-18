option(ENABLE_SIGNING "Sign the executable with a code signing certificate" OFF)

set(SOURCES
    main.cpp
    Core/Logger.cpp
    Core/SettingsManager.cpp
    Core/AppStateMachine.cpp
    Core/DeviceManager.cpp
    Core/InputManager.cpp
    Core/CursorManager.cpp
    Core/ClickForwarder.cpp
    Platform/RawInputHandler.cpp
    Platform/WindowManager.cpp
    Rendering/Direct2DRenderer.cpp
    Rendering/OverlayRenderer.cpp
    UI/MainWindow.cpp
    UI/SettingsWindow.cpp
)

add_executable(MultiCursor WIN32 ${SOURCES} ${CMAKE_SOURCE_DIR}/resources/app.rc)

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
    wtsapi32.lib
    dwmapi.lib
    dbghelp.lib
)

set_target_properties(MultiCursor PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(ENABLE_SIGNING)
    find_program(SIGNTOOL signtool.exe PATHS
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/10.0.26100.0/x64"
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/10.0.20348.0/x64"
        "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/10.0.19041.0/x64"
    )
    if(SIGNTOOL)
        set(PFX_PATH "${CMAKE_SOURCE_DIR}/resources/MultiCursorDev.pfx")
        set(PFX_PASSWORD "MultiCursorDev2026")
        add_custom_command(TARGET MultiCursor POST_BUILD
            COMMAND "${SIGNTOOL}" sign /fd SHA256 /a
                    /f "${PFX_PATH}" /p "${PFX_PASSWORD}"
                    "$<TARGET_FILE:MultiCursor>"
            COMMENT "Signing MultiCursor.exe with development certificate"
        )
        message(STATUS "Code signing enabled: ${SIGNTOOL}")
    else()
        message(WARNING "ENABLE_SIGNING is ON but signtool.exe not found. Install Windows SDK.")
    endif()
endif()
