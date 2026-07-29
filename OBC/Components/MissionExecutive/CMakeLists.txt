####
# MissionExecutive module and unit/integration tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/MissionExecutive.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/MissionExecutivePolicy.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/MissionExecutive.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/MissionExecutive.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/MissionExecutivePolicy.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/MissionExecutiveRuntime.hpp"
  DEPENDS
    OBC_Types
)
target_include_directories(OBC_Components_MissionExecutive PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/MissionExecutiveTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/MissionExecutiveTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/MissionExecutive.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/MissionExecutiveTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_MissionExecutive_ut_exe)
  target_include_directories(OBC_Components_MissionExecutive_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()

if (BUILD_TESTING)
  add_executable(mission_executive_policy_integration_test
    "${CMAKE_CURRENT_LIST_DIR}/MissionExecutivePolicy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/integration/MissionExecutivePolicyIntegrationTest.cpp"
  )
  target_compile_features(mission_executive_policy_integration_test PRIVATE cxx_std_14)
  target_include_directories(mission_executive_policy_integration_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(mission_executive_policy_integration_test
    PRIVATE
      obc_scenario_replay
  )
  add_test(NAME mission_executive_policy_integration_test COMMAND mission_executive_policy_integration_test)
endif()
