####
# TtcPassManager module and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/TtcPassManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/TtcPassPolicy.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/TtcPassManager.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/TtcPassManager.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/TtcPassPolicy.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/TtcPassRuntime.hpp"
  DEPENDS
    OBC_Types
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/TtcPassManagerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/TtcPassManagerTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/TtcPassManager.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/TtcPassManagerTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)

if (BUILD_TESTING)
  add_executable(ttc_pass_policy_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/TtcPassPolicy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/TtcPassPolicyUnitTest.cpp"
  )
  target_compile_features(ttc_pass_policy_unit_test PRIVATE cxx_std_14)
  target_include_directories(ttc_pass_policy_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(ttc_pass_policy_unit_test
    PRIVATE
      OBC_Types
      Fw_StringFormat_snprintf
  )
  add_test(NAME ttc_pass_policy_unit_test COMMAND ttc_pass_policy_unit_test)
endif()
