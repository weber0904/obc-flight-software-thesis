
register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/StorageHealthBridge.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/StorageScanner.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/StorageHealthBridge.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/StorageHealthBridge.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/StorageHealthTypes.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/StorageScanner.hpp"
  DEPENDS
    OBC_Types
)
target_include_directories(OBC_Components_StorageHealthBridge PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/StorageHealthBridgeTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/StorageHealthBridgeTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/StorageHealthBridge.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/StorageHealthBridgeTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_StorageHealthBridge_ut_exe)
  target_include_directories(OBC_Components_StorageHealthBridge_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(OBC_Components_StorageHealthBridge_ut_exe
    PRIVATE
      Fw_StringFormat_snprintf
  )
endif()

if (BUILD_TESTING)
  add_executable(storage_health_bridge_contract_test
    "${CMAKE_CURRENT_LIST_DIR}/test/StorageHealthBridgeContractTest.cpp"
  )
  target_compile_features(storage_health_bridge_contract_test PRIVATE cxx_std_14)
  target_include_directories(storage_health_bridge_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(storage_health_bridge_contract_test
    PRIVATE
      OBC_Components_StorageHealthBridge
      Fw_StringFormat_snprintf
  )
  add_test(NAME storage_health_bridge_contract_test COMMAND storage_health_bridge_contract_test)

  add_executable(storage_scanner_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/StorageScanner.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/StorageScannerUnitTest.cpp"
  )
  target_compile_features(storage_scanner_unit_test PRIVATE cxx_std_14)
  target_include_directories(storage_scanner_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(storage_scanner_unit_test
    PRIVATE
      OBC_Components_StorageHealthBridge
      Fw_StringFormat_snprintf
  )
  add_test(NAME storage_scanner_unit_test COMMAND storage_scanner_unit_test)

  add_executable(storage_health_bridge_cached_state_integration_test
    "${CMAKE_CURRENT_LIST_DIR}/test/integration/StorageHealthBridgeCachedStateIntegrationTest.cpp"
  )
  target_compile_features(storage_health_bridge_cached_state_integration_test PRIVATE cxx_std_14)
  target_include_directories(storage_health_bridge_cached_state_integration_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(storage_health_bridge_cached_state_integration_test
    PRIVATE
      OBC_Components_StorageHealthBridge
      Fw_StringFormat_snprintf
  )
  add_test(NAME storage_health_bridge_cached_state_integration_test COMMAND storage_health_bridge_cached_state_integration_test)
endif()
