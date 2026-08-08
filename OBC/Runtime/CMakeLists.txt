add_library(OBC_Runtime STATIC
  "${CMAKE_CURRENT_LIST_DIR}/HostedRuntime.cpp"
)
target_compile_features(OBC_Runtime PUBLIC cxx_std_14)
target_include_directories(OBC_Runtime
  PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/comm"
)
target_link_libraries(OBC_Runtime
  PUBLIC
    OBC_Components_AdcsBridge
    OBC_Components_BootManager
    OBC_Components_CommandIngressAuthority
    OBC_Components_CommController
    OBC_Components_CspBridge
    OBC_Components_EpsBridge
    OBC_Components_GpsBridge
    OBC_Components_GroundLinkDriver
    OBC_Components_ModeManager
    OBC_Components_RadioController
    OBC_Components_StorageHealthBridge
    OBC_Components_UartDriver
    OBC_Components_WatchdogSupervisor
    obc_comm_groundlink
    obc_comm_transport
)

if (BUILD_TESTING)
  add_executable(hosted_runtime_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/test/HostedRuntimeUnitTest.cpp"
  )
  target_compile_features(hosted_runtime_unit_test PRIVATE cxx_std_14)
  target_include_directories(hosted_runtime_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/comm"
  )
  target_link_libraries(hosted_runtime_unit_test
    PRIVATE
      OBC_Runtime
      Fw_StringFormat_snprintf
  )
  add_test(NAME hosted_runtime_unit_test COMMAND hosted_runtime_unit_test)
endif()
