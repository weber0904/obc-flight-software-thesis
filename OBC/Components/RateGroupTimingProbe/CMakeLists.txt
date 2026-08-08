####
# RateGroupTimingProbe component
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/RateGroupTimingProbe.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/RateGroupTimingProbe.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/RateGroupTimingProbe.hpp"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/RateGroupTimingProbeTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/RateGroupTimingProbeTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/RateGroupTimingProbe.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/RateGroupTimingProbeTester.hpp"
  UT_AUTO_HELPERS
)
