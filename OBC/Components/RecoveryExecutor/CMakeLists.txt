####
# RecoveryExecutor module and focused tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/RecoveryExecutor.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/RecoveryExecutor.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/RecoveryExecutor.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/RecoveryRuntime.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_BootManager
    OBC_Components_EpsBridge
    OBC_Components_ModeSafetyController
    OBC_Components_PersistentFaultManager
)
target_include_directories(OBC_Components_RecoveryExecutor PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/RecoveryExecutorTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/RecoveryExecutorTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/RecoveryExecutor.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/RecoveryExecutorTester.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_AdcsBridge
    OBC_Components_BootManager
    OBC_Components_EpsBridge
    OBC_Components_ModeSafetyController
    OBC_Components_PersistentFaultManager
    Os
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_RecoveryExecutor_ut_exe)
  target_include_directories(OBC_Components_RecoveryExecutor_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()

if (BUILD_TESTING)
  add_executable(recovery_runtime_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/test/RecoveryRuntimeUnitTest.cpp"
  )
  target_compile_features(recovery_runtime_unit_test PRIVATE cxx_std_14)
  target_include_directories(recovery_runtime_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(recovery_runtime_unit_test
    PRIVATE
      OBC_Types
      Fw_StringFormat_snprintf
  )
  add_test(NAME recovery_runtime_unit_test COMMAND recovery_runtime_unit_test)
endif()
