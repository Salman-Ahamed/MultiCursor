find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    find_package(GTest QUIET)
endif()

if(GTest_FOUND)
    add_executable(MultiCursorTests
        test_main.cpp
    )
    target_include_directories(MultiCursorTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(MultiCursorTests PRIVATE GTest::GTest GTest::Main)
    add_test(NAME MultiCursorTests COMMAND MultiCursorTests)
else()
    message(STATUS "GTest not found - tests disabled. Install Google Test or set GTEST_ROOT.")
endif()
