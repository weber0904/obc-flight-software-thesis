####
# EpsFdirController module and focused tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/EpsFdirController.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/EpsFdirPolicy.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/EpsFdirController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/EpsFdirController.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/EpsFdirPolicy.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/EpsFdirRuntime.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_ModeSafetyController
)
target_include_directories(OBC_Components_EpsFdirController PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/EpsFdirControllerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/EpsFdirControllerTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/EpsFdirController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/EpsFdirControllerTester.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_ModeSafetyController
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_EpsFdirController_ut_exe)
  target_include_directories(OBC_Components_EpsFdirController_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()

if (BUILD_TESTING)
  add_executable(eps_fdir_policy_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/EpsFdirPolicy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/EpsFdirPolicyUnitTest.cpp"
  )
  target_compile_features(eps_fdir_policy_unit_test PRIVATE cxx_std_14)
  target_include_directories(eps_fdir_policy_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(eps_fdir_policy_unit_test
    PRIVATE
      OBC_Types
      Fw_StringFormat_snprintf
  )
  add_test(NAME eps_fdir_policy_unit_test COMMAND eps_fdir_policy_unit_test)

  add_executable(eps_fdir_integration_test
    "${CMAKE_CURRENT_LIST_DIR}/test/integration/EpsFdirIntegrationTest.cpp"
  )
  target_compile_features(eps_fdir_integration_test PRIVATE cxx_std_14)
  target_include_directories(eps_fdir_integration_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/common"
    "${PROJECT_SOURCE_DIR}/simulators/eps"
  )
  target_link_libraries(eps_fdir_integration_test
    PRIVATE
      OBC_Components_BootManager
      OBC_Components_EpsBridge
      OBC_Components_EpsFdirController
      OBC_Components_ModeManager
      OBC_Components_RecoveryExecutor
      Os
      Os_File
      Os_File_Posix
      obc_eps_transport
      Fw_StringFormat_snprintf
  )
  add_test(NAME eps_fdir_integration_test COMMAND eps_fdir_integration_test)
endif()
