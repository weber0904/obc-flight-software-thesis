register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/LinuxWatchdogSink.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/LinuxWatchdogSink.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/LinuxWatchdogSink.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/LinuxWatchdogRuntime.hpp"
  DEPENDS
    OBC_Types
)
target_include_directories(OBC_Components_LinuxWatchdogSink PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/LinuxWatchdogSinkTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/LinuxWatchdogSinkTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/LinuxWatchdogSink.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/LinuxWatchdogSinkTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_LinuxWatchdogSink_ut_exe)
  target_include_directories(OBC_Components_LinuxWatchdogSink_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()
