####
# WatchdogSupervisor module and focused tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/WatchdogSupervisor.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/WatchdogPolicy.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/WatchdogSupervisor.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/WatchdogSupervisor.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/WatchdogRuntime.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/WatchdogPolicy.hpp"
  DEPENDS
    OBC_Types
)
target_include_directories(OBC_Components_WatchdogSupervisor PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/WatchdogSupervisorTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/WatchdogSupervisorTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/WatchdogSupervisor.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/WatchdogSupervisorTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_WatchdogSupervisor_ut_exe)
  target_include_directories(OBC_Components_WatchdogSupervisor_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()

if (BUILD_TESTING)
  add_executable(watchdog_policy_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/WatchdogPolicy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/WatchdogPolicyUnitTest.cpp"
  )
  target_compile_features(watchdog_policy_unit_test PRIVATE cxx_std_14)
  target_include_directories(watchdog_policy_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(watchdog_policy_unit_test
    PRIVATE
      OBC_Types
      Fw_StringFormat_snprintf
  )
  add_test(NAME watchdog_policy_unit_test COMMAND watchdog_policy_unit_test)
endif()
