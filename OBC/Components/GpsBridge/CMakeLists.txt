####
# GpsBridge module
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/GpsBridge.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/GpsBridge.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/GpsBridge.hpp"
  DEPENDS
    OBC_Types
)
target_link_libraries(OBC_Components_GpsBridge PRIVATE obc_gps_support)
target_include_directories(OBC_Components_GpsBridge PRIVATE
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/simulators/gps"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/GpsBridgeTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/GpsBridgeTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/GpsBridge.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/GpsBridgeTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_GpsBridge_ut_exe)
  target_include_directories(OBC_Components_GpsBridge_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/gps"
  )
  target_link_libraries(OBC_Components_GpsBridge_ut_exe
    PRIVATE
      obc_gps_support
      Fw_StringFormat_snprintf
  )
endif()

if (BUILD_TESTING)
  add_executable(gps_bridge_contract_test
    "${CMAKE_CURRENT_LIST_DIR}/test/GpsBridgeContractTest.cpp"
  )
  target_compile_features(gps_bridge_contract_test PRIVATE cxx_std_14)
  target_include_directories(gps_bridge_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/gps"
  )
  target_link_libraries(gps_bridge_contract_test
    PRIVATE
      OBC_Components_GpsBridge
      obc_gps_support
      Fw_StringFormat_snprintf
  )
  add_test(NAME gps_bridge_contract_test COMMAND gps_bridge_contract_test)

  add_executable(gps_bridge_cached_state_integration_test
    "${CMAKE_CURRENT_LIST_DIR}/test/integration/GpsBridgeCachedStateIntegrationTest.cpp"
  )
  target_compile_features(gps_bridge_cached_state_integration_test PRIVATE cxx_std_14)
  target_include_directories(gps_bridge_cached_state_integration_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/gps"
  )
  target_link_libraries(gps_bridge_cached_state_integration_test
    PRIVATE
      OBC_Components_GpsBridge
      obc_gps_support
      Fw_StringFormat_snprintf
  )
  add_test(NAME gps_bridge_cached_state_integration_test COMMAND gps_bridge_cached_state_integration_test)
endif()
