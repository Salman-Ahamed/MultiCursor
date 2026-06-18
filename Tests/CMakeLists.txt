set(TEST_SOURCES
    test_main.cpp
    minitest.h
    EventBusTest.cpp
    AppStateMachineTest.cpp
    SettingsManagerTest.cpp
    InputManagerTest.cpp
)

# Source files from main project (excluding GPU/UI/Platform files not needed for tests)
set(PROJECT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/Core/SettingsManager.cpp
    ${CMAKE_SOURCE_DIR}/src/Core/AppStateMachine.cpp
    ${CMAKE_SOURCE_DIR}/src/Core/InputManager.cpp
    ${CMAKE_SOURCE_DIR}/src/Core/Logger.cpp
)

add_executable(MultiCursorTests ${TEST_SOURCES} ${PROJECT_SOURCES})
target_include_directories(MultiCursorTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(MultiCursorTests PRIVATE user32.lib hid.lib ole32.lib oleaut32.lib)
add_test(NAME MultiCursorTests COMMAND MultiCursorTests)
