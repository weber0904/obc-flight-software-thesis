####
# ModeManager module and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ModeManager.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/ModeManager.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ModeManager.hpp"
  DEPENDS
    OBC_Types
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/ModeManagerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/ModeManagerTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/ModeManager.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/ModeManagerTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
