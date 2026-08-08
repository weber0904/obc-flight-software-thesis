####
# ModeSafetyController module and focused tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/ModeSafetyController.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/ModeSafetyPolicy.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/ModeSafetyController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/ModeSafetyController.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ModeSafetyPolicy.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/ModeSafetyRuntime.hpp"
  DEPENDS
    OBC_Types
)
target_include_directories(OBC_Components_ModeSafetyController PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/ModeSafetyControllerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/ModeSafetyControllerTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/ModeSafetyController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/ModeSafetyControllerTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_ModeSafetyController_ut_exe)
  target_include_directories(OBC_Components_ModeSafetyController_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()

if (BUILD_TESTING)
  add_executable(mode_safety_policy_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/ModeSafetyPolicy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ModeSafetyPolicyUnitTest.cpp"
  )
  target_compile_features(mode_safety_policy_unit_test PRIVATE cxx_std_14)
  target_include_directories(mode_safety_policy_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(mode_safety_policy_unit_test
    PRIVATE
      OBC_Types
      Fw_StringFormat_snprintf
  )
  add_test(NAME mode_safety_policy_unit_test COMMAND mode_safety_policy_unit_test)

  add_executable(mode_safety_policy_integration_test
    "${CMAKE_CURRENT_LIST_DIR}/test/integration/ModeSafetyPolicyIntegrationTest.cpp"
  )
  target_compile_features(mode_safety_policy_integration_test PRIVATE cxx_std_14)
  target_include_directories(mode_safety_policy_integration_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(mode_safety_policy_integration_test
    PRIVATE
      OBC_Components_ModeSafetyController
      OBC_Components_ModeManager
      Fw_StringFormat_snprintf
  )
  add_test(NAME mode_safety_policy_integration_test COMMAND mode_safety_policy_integration_test)
endif()
