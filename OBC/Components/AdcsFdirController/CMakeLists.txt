####
# AdcsFdirController module and focused tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/AdcsFdirController.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/AdcsFdirPolicy.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/AdcsFdirController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/AdcsFdirController.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/AdcsFdirPolicy.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/AdcsFdirRuntime.hpp"
  DEPENDS
    OBC_Types
)
target_include_directories(OBC_Components_AdcsFdirController PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/AdcsFdirControllerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/AdcsFdirControllerTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/AdcsFdirController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/AdcsFdirControllerTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_AdcsFdirController_ut_exe)
  target_include_directories(OBC_Components_AdcsFdirController_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()

if (BUILD_TESTING)
  add_executable(adcs_fdir_policy_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/AdcsFdirPolicy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/AdcsFdirPolicyUnitTest.cpp"
  )
  target_compile_features(adcs_fdir_policy_unit_test PRIVATE cxx_std_14)
  target_include_directories(adcs_fdir_policy_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(adcs_fdir_policy_unit_test
    PRIVATE
      OBC_Types
      Fw_StringFormat_snprintf
  )
  add_test(NAME adcs_fdir_policy_unit_test COMMAND adcs_fdir_policy_unit_test)
endif()
