####
# UhfCcsdsVcidAdapter component and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/UhfCcsdsVcidAdapter.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/UhfCcsdsVcidAdapter.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/UhfCcsdsVcidAdapter.hpp"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/UhfCcsdsVcidAdapterTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/UhfCcsdsVcidAdapterTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/UhfCcsdsVcidAdapter.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/UhfCcsdsVcidAdapterTester.hpp"
  UT_AUTO_HELPERS
)
