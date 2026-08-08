

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/OBCComCcsdsConfig/OBCComCcsdsSubtopologyConfig.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/OnboardStateSnapshotSource.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/OBCAppTopology.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PayloadCspGateway.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PayloadCspService.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/instances.fpp"
    "${CMAKE_CURRENT_LIST_DIR}/OBCComCcsds.fpp"
    "${CMAKE_CURRENT_LIST_DIR}/OBCComCcsdsConfig/OBCComCcsdsConfig.fpp"
    "${CMAKE_CURRENT_LIST_DIR}/topology.fpp"
  DEPENDS
    Svc_ActiveRateGroup
    Svc_LinuxTimer
    Svc_PassiveConsoleTextLogger
    Svc_PosixTime
    Svc_RateGroupDriver
    Svc_BufferManager
    Svc_DpCatalog
    Svc_DpManager
    Svc_DpWriter
    Svc_Subtopologies_CdhCore
    Svc_Subtopologies_ComCcsds
    Svc_Subtopologies_ComFprime
    Svc_Subtopologies_FileHandling
    OBC_Components_BeaconPublisher
    OBC_Components_CommandIngressAuthority
    OBC_Components_CommandIngressMux
    OBC_Components_CommController
    OBC_Components_DpCatalogFileDownlinkGate
    OBC_Components_HkTrendProductProducer
    OBC_Components_OnboardStateData
    OBC_Components_OnboardStateMonitor
    OBC_Components_PayloadOpsController
    OBC_Components_SecureLinkAuthProtocol
    OBC_Components_SecureLinkAuthorizer
    OBC_Components_UhfCcsdsVcidAdapter
    obc_csp_runtime
)

if (BUILD_TESTING)
  add_executable(payload_csp_gateway_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/PayloadCspGateway.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/PayloadCspGatewayUnitTest.cpp"
  )
  target_compile_features(payload_csp_gateway_unit_test PRIVATE cxx_std_14)
  target_include_directories(payload_csp_gateway_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(payload_csp_gateway_unit_test
    PRIVATE
      gtest_main
      Fw_Types
      Fw_StringFormat_snprintf
      OBC_Components_PayloadOpsController
  )
  add_test(NAME payload_csp_gateway_unit_test COMMAND payload_csp_gateway_unit_test)

  add_executable(onboard_state_snapshot_source_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/OnboardStateSnapshotSource.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/OnboardStateSnapshotSourceUnitTest.cpp"
  )
  target_compile_features(onboard_state_snapshot_source_unit_test PRIVATE cxx_std_14)
  target_include_directories(onboard_state_snapshot_source_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/gps"
  )
  target_link_libraries(onboard_state_snapshot_source_unit_test
    PRIVATE
      OBC_Components_EpsBridge
      OBC_Components_AdcsBridge
      OBC_Components_ModeManager
      OBC_Components_CommController
      OBC_Components_CspBridge
      OBC_Components_RadioController
      OBC_Components_UartDriver
      OBC_Components_BootManager
      OBC_Components_GpsBridge
      OBC_Components_StorageHealthBridge
      OBC_Components_OnboardStateData
      obc_gps_support
      Fw_StringFormat_snprintf
      Os_File
      Os_File_Posix
      Os_Mutex_Posix
      Os_Mutex_Posix_Implementation
      Os_Posix_Shared
  )
  add_test(NAME onboard_state_snapshot_source_unit_test COMMAND onboard_state_snapshot_source_unit_test)
endif()

add_executable(payload_csp_probe_main
  "${CMAKE_CURRENT_LIST_DIR}/test/PayloadCspProbeMain.cpp"
)
target_compile_features(payload_csp_probe_main PRIVATE cxx_std_14)
target_include_directories(payload_csp_probe_main PRIVATE
  "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(payload_csp_probe_main
  PRIVATE
    obc_csp_runtime
)
