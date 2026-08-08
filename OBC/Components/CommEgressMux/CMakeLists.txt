####
# CommEgressMux component and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/CommEgressMux.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CommEgressMux.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/CommEgressMux.hpp"
  DEPENDS
    OBC_Types
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommEgressMuxTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommEgressMuxTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CommEgressMux.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommEgressMuxTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
