
register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/OnboardStateMonitor.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/OnboardStateMonitor.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/OnboardStateMonitor.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_OnboardStateData
)
target_include_directories(OBC_Components_OnboardStateMonitor PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/OnboardStateMonitorTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/OnboardStateMonitorTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/OnboardStateMonitor.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/OnboardStateMonitorTester.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_OnboardStateData
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_OnboardStateMonitor_ut_exe)
  target_include_directories(OBC_Components_OnboardStateMonitor_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()
