# CMake generated Testfile for 
# Source directory: D:/codes/work/org/Multi-Mouse-Detection/Tests
# Build directory: D:/codes/work/org/Multi-Mouse-Detection/build/Tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[MultiCursorTests]=] "D:/codes/work/org/Multi-Mouse-Detection/build/Tests/Debug/MultiCursorTests.exe")
  set_tests_properties([=[MultiCursorTests]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/codes/work/org/Multi-Mouse-Detection/Tests/CMakeLists.txt;21;add_test;D:/codes/work/org/Multi-Mouse-Detection/Tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[MultiCursorTests]=] "D:/codes/work/org/Multi-Mouse-Detection/build/Tests/Release/MultiCursorTests.exe")
  set_tests_properties([=[MultiCursorTests]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/codes/work/org/Multi-Mouse-Detection/Tests/CMakeLists.txt;21;add_test;D:/codes/work/org/Multi-Mouse-Detection/Tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[MultiCursorTests]=] "D:/codes/work/org/Multi-Mouse-Detection/build/Tests/MinSizeRel/MultiCursorTests.exe")
  set_tests_properties([=[MultiCursorTests]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/codes/work/org/Multi-Mouse-Detection/Tests/CMakeLists.txt;21;add_test;D:/codes/work/org/Multi-Mouse-Detection/Tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[MultiCursorTests]=] "D:/codes/work/org/Multi-Mouse-Detection/build/Tests/RelWithDebInfo/MultiCursorTests.exe")
  set_tests_properties([=[MultiCursorTests]=] PROPERTIES  _BACKTRACE_TRIPLES "D:/codes/work/org/Multi-Mouse-Detection/Tests/CMakeLists.txt;21;add_test;D:/codes/work/org/Multi-Mouse-Detection/Tests/CMakeLists.txt;0;")
else()
  add_test([=[MultiCursorTests]=] NOT_AVAILABLE)
endif()
