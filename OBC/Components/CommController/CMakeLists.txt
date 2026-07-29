####
# CommController module and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/CommController.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommFdirPolicy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommDownlinkScheduler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommReliableTransfer.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CommController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/CommController.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommBeaconSuppressControl.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommFdirPolicy.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommDownlinkScheduler.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommReliableTransfer.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_CommandIngressAuthority
    OBC_Components_FileIngressControlProtocol
    OBC_Components_SecureLinkAuthProtocol
    OBC_Components_GroundLinkDriver
    OBC_Components_GroundLinkHealthProvider
    OBC_Components_CommEgressMux
    Svc_FileDownlinkPorts
)
target_link_libraries(OBC_Components_CommController PRIVATE obc_csp_runtime)
target_include_directories(OBC_Components_CommController PRIVATE
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/simulators/comm"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommControllerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommControllerTestMain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/CommDownlinkSchedulerTest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/CommReliableTransferTest.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CommController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommControllerTester.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_CommandIngressAuthority
    OBC_Components_FileIngressControlProtocol
    OBC_Components_SecureLinkAuthProtocol
    OBC_Components_GroundLinkDriver
    OBC_Components_GroundLinkHealthProvider
    OBC_Components_CommEgressMux
    Svc_FileDownlinkPorts
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_CommController_ut_exe)
  target_link_libraries(OBC_Components_CommController_ut_exe PRIVATE obc_csp_runtime)
  target_include_directories(OBC_Components_CommController_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/comm"
  )
endif()

if (BUILD_TESTING)
  add_executable(comm_fdir_policy_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/CommFdirPolicy.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/CommFdirPolicyUnitTest.cpp"
  )
  target_compile_features(comm_fdir_policy_unit_test PRIVATE cxx_std_14)
  target_include_directories(comm_fdir_policy_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(comm_fdir_policy_unit_test
    PRIVATE
      OBC_Types
      Fw_StringFormat_snprintf
  )
  add_test(NAME comm_fdir_policy_unit_test COMMAND comm_fdir_policy_unit_test)

  add_executable(comm_reliable_transfer_protocol_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/test/CommReliableTransferProtocolTest.cpp"
  )
  target_compile_features(comm_reliable_transfer_protocol_unit_test PRIVATE cxx_std_14)
  target_include_directories(comm_reliable_transfer_protocol_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/comm"
  )
  target_link_libraries(comm_reliable_transfer_protocol_unit_test
    PRIVATE
      gtest_main
  )
  add_test(NAME comm_reliable_transfer_protocol_unit_test COMMAND comm_reliable_transfer_protocol_unit_test)
endif()
