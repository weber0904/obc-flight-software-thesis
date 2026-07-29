find_package(Threads REQUIRED)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBSOCKETCAN REQUIRED IMPORTED_TARGET libsocketcan)
endif()

find_path(ZMQ_INCLUDE_DIR
  NAMES zmq.h
  HINTS /opt/homebrew/include /usr/local/include
)

find_library(ZMQ_LIBRARY
  NAMES zmq
  HINTS /opt/homebrew/lib /usr/local/lib
)

if (NOT ZMQ_INCLUDE_DIR OR NOT ZMQ_LIBRARY)
  message(FATAL_ERROR "ZeroMQ headers and library are required for the hosted simulators")
endif()

set(OBC_LIBCSP_DIR "${PROJECT_SOURCE_DIR}/lib/libcsp")
set(OBC_LIBCSP_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/libcsp/include")
set(OBC_LIBCSP_PATCHED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/libcsp/patched")
file(MAKE_DIRECTORY "${OBC_LIBCSP_GENERATED_INCLUDE_DIR}/csp")
file(MAKE_DIRECTORY "${OBC_LIBCSP_PATCHED_DIR}")
file(COPY "${OBC_LIBCSP_DIR}/src" DESTINATION "${OBC_LIBCSP_PATCHED_DIR}")

set(CSP_POSIX 1)
set(CSP_ZEPHYR 0)
set(CSP_FREERTOS 0)
set(CSP_HAVE_STDIO 1)
set(CSP_ENABLE_CSP_PRINT 1)
set(CSP_PRINT_STDIO 1)
set(CSP_REPRODUCIBLE_BUILDS 0)
set(CSP_QFIFO_LEN 32)
# ADCS app services use ports 20..24 and COMM reserves ports 30..39.
set(CSP_PORT_MAX_BIND 41)
set(CSP_CONN_RXQUEUE_LEN 32)
set(CSP_CONN_MAX 16)
set(CSP_BUFFER_SIZE 2048)
set(CSP_BUFFER_COUNT 24)
set(CSP_RDP_MAX_WINDOW 5)
set(CSP_RTABLE_SIZE 10)
set(CSP_USE_RDP 0)
set(CSP_USE_HMAC 0)
set(CSP_USE_PROMISC 0)
set(CSP_USE_RTABLE 1)
set(CSP_BUFFER_ZERO_CLEAR 1)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CSP_HAVE_LIBSOCKETCAN 1)
else()
  set(CSP_HAVE_LIBSOCKETCAN 0)
endif()
set(CSP_HAVE_LIBZMQ 1)
set(CSP_FIXUP_V1_ZMQ_LITTLE_ENDIAN 1)
set(CSP_ENABLE_KISS_CRC 1)

configure_file(
  "${OBC_LIBCSP_DIR}/csp_autoconfig.h.in"
  "${OBC_LIBCSP_GENERATED_INCLUDE_DIR}/csp/autoconfig.h"
)

file(READ "${OBC_LIBCSP_DIR}/src/csp_macro.h" OBC_LIBCSP_MACRO_CONTENT)
string(REPLACE
  "#define __noinit __attribute__((section(\".noinit\")))"
  "#ifdef __APPLE__\n#define __noinit\n#else\n#define __noinit __attribute__((section(\".noinit\")))\n#endif"
  OBC_LIBCSP_MACRO_CONTENT
  "${OBC_LIBCSP_MACRO_CONTENT}"
)
string(REPLACE
  "#define __weak   __attribute__((__weak__))"
  "#ifdef __APPLE__\n#ifdef __weak\n#undef __weak\n#endif\n#endif\n#define __weak   __attribute__((__weak__))"
  OBC_LIBCSP_MACRO_CONTENT
  "${OBC_LIBCSP_MACRO_CONTENT}"
)
file(WRITE "${OBC_LIBCSP_PATCHED_DIR}/src/csp_macro.h" "${OBC_LIBCSP_MACRO_CONTENT}")

file(READ "${OBC_LIBCSP_PATCHED_DIR}/src/csp_id.c" OBC_LIBCSP_ID_CONTENT)
string(REPLACE
  "packet->frame_begin = packet->data - CSP_ID1_HEADER_SIZE;"
  "packet->frame_begin = &packet->header[CSP_PACKET_PADDING_BYTES - CSP_ID1_HEADER_SIZE];"
  OBC_LIBCSP_ID_CONTENT
  "${OBC_LIBCSP_ID_CONTENT}"
)
string(REPLACE
  "packet->frame_begin = packet->data - CSP_ID2_HEADER_SIZE;"
  "packet->frame_begin = &packet->header[CSP_PACKET_PADDING_BYTES - CSP_ID2_HEADER_SIZE];"
  OBC_LIBCSP_ID_CONTENT
  "${OBC_LIBCSP_ID_CONTENT}"
)
file(WRITE "${OBC_LIBCSP_PATCHED_DIR}/src/csp_id.c" "${OBC_LIBCSP_ID_CONTENT}")

set(OBC_LIBCSP_CORE_SOURCES
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_bridge.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_buffer.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_conn.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_crc32.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_debug.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_dedup.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_id.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_iflist.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_init.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_io.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_port.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_qfifo.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_route.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_service_handler.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_services.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_sfp.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/csp_rtable_cidr.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/interfaces/csp_if_lo.c"
  "${OBC_LIBCSP_PATCHED_DIR}/src/interfaces/csp_if_zmqhub.c"
)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND OBC_LIBCSP_CORE_SOURCES
    "${OBC_LIBCSP_PATCHED_DIR}/src/interfaces/csp_if_can.c"
    "${OBC_LIBCSP_PATCHED_DIR}/src/interfaces/csp_if_can_pbuf.c"
    "${OBC_LIBCSP_PATCHED_DIR}/src/drivers/can/can_socketcan.c"
  )
endif()

if (APPLE)
  list(APPEND OBC_LIBCSP_CORE_SOURCES
    "${OBC_LIBCSP_PATCHED_DIR}/src/arch/posix/csp_clock.c"
    "${OBC_LIBCSP_PATCHED_DIR}/src/arch/posix/csp_queue.c"
    "${OBC_LIBCSP_PATCHED_DIR}/src/arch/posix/csp_time.c"
    "${CMAKE_CURRENT_LIST_DIR}/csp/csp_semaphore_darwin.c"
    "${CMAKE_CURRENT_LIST_DIR}/csp/pthread_queue_darwin.c"
    "${CMAKE_CURRENT_LIST_DIR}/csp/csp_system_darwin.c"
  )
else()
  list(APPEND OBC_LIBCSP_CORE_SOURCES
    "${OBC_LIBCSP_PATCHED_DIR}/src/arch/posix/csp_clock.c"
    "${OBC_LIBCSP_PATCHED_DIR}/src/arch/posix/csp_queue.c"
    "${OBC_LIBCSP_PATCHED_DIR}/src/arch/posix/csp_semaphore.c"
    "${OBC_LIBCSP_PATCHED_DIR}/src/arch/posix/csp_system.c"
    "${OBC_LIBCSP_PATCHED_DIR}/src/arch/posix/csp_time.c"
    "${OBC_LIBCSP_PATCHED_DIR}/src/arch/posix/pthread_queue.c"
  )
endif()

add_library(obc_libcsp STATIC
  ${OBC_LIBCSP_CORE_SOURCES}
)
if (NOT APPLE)
  target_compile_definitions(obc_libcsp
    PRIVATE
      _DEFAULT_SOURCE
      _POSIX_C_SOURCE=200809L
  )
endif()
target_include_directories(obc_libcsp
  PUBLIC
    "${OBC_LIBCSP_DIR}/include"
    "${OBC_LIBCSP_GENERATED_INCLUDE_DIR}"
  PRIVATE
    "${PROJECT_SOURCE_DIR}/cmake/compat"
    "${OBC_LIBCSP_PATCHED_DIR}/src"
    "${OBC_LIBCSP_PATCHED_DIR}/src/arch/posix"
    "${ZMQ_INCLUDE_DIR}"
)
target_link_libraries(obc_libcsp
  PUBLIC
    Threads::Threads
    "${ZMQ_LIBRARY}"
)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(obc_libcsp PUBLIC PkgConfig::LIBSOCKETCAN)
endif()

add_library(obc_csp_runtime STATIC
  "${CMAKE_CURRENT_LIST_DIR}/csp/CspRuntime.cpp"
)
target_compile_features(obc_csp_runtime PUBLIC cxx_std_14)
target_include_directories(obc_csp_runtime
  PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/csp"
  PRIVATE
    "${OBC_LIBCSP_PATCHED_DIR}/src"
)
target_link_libraries(obc_csp_runtime
  PUBLIC
    obc_libcsp
)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(obc_csp_runtime PUBLIC OBC_CSP_HAVE_SOCKETCAN=1)
endif()

add_executable(csp_zmqproxy
  "${OBC_LIBCSP_DIR}/examples/zmqproxy.c"
)
if (NOT APPLE)
  target_compile_definitions(csp_zmqproxy
    PRIVATE
      _DEFAULT_SOURCE
      _POSIX_C_SOURCE=200809L
  )
endif()
target_include_directories(csp_zmqproxy
  PRIVATE
    "${OBC_LIBCSP_DIR}/include"
    "${OBC_LIBCSP_GENERATED_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/cmake/compat"
    "${ZMQ_INCLUDE_DIR}"
)
target_link_libraries(csp_zmqproxy
  PRIVATE
    obc_libcsp
    Threads::Threads
    "${ZMQ_LIBRARY}"
)

add_executable(csp_service_peer
  "${CMAKE_CURRENT_LIST_DIR}/csp/CspServicePeerMain.cpp"
)
target_compile_features(csp_service_peer PRIVATE cxx_std_14)
target_link_libraries(csp_service_peer PRIVATE obc_csp_runtime Threads::Threads)

add_executable(csp_runtime_smoke
  "${CMAKE_CURRENT_LIST_DIR}/csp/CspRuntimeSmokeMain.cpp"
)
target_compile_features(csp_runtime_smoke PRIVATE cxx_std_14)
target_link_libraries(csp_runtime_smoke PRIVATE obc_csp_runtime)

add_library(obc_eps_protocol INTERFACE)
target_compile_features(obc_eps_protocol INTERFACE cxx_std_14)
target_include_directories(obc_eps_protocol
  INTERFACE
    "${PROJECT_SOURCE_DIR}"
)

add_library(obc_eps_transport STATIC
  "${CMAKE_CURRENT_LIST_DIR}/eps/EpsTransport.cpp"
)
target_compile_features(obc_eps_transport PUBLIC cxx_std_14)
target_include_directories(obc_eps_transport
  PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/eps"
)
target_link_libraries(obc_eps_transport
  PUBLIC
    obc_eps_protocol
    obc_csp_runtime
)

add_library(obc_eps_sim STATIC
  "${CMAKE_CURRENT_LIST_DIR}/eps/EpsSimModel.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/eps/EpsSimServer.cpp"
)
target_compile_features(obc_eps_sim PUBLIC cxx_std_14)
target_include_directories(obc_eps_sim
  PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/eps"
)
target_link_libraries(obc_eps_sim
  PUBLIC
    obc_eps_protocol
    obc_csp_runtime
    Threads::Threads
)

add_executable(eps_simulator
  "${CMAKE_CURRENT_LIST_DIR}/eps/EpsSimulatorMain.cpp"
)
target_compile_features(eps_simulator PRIVATE cxx_std_14)
target_link_libraries(eps_simulator PRIVATE obc_eps_sim)

add_library(obc_adcs_transport STATIC
  "${CMAKE_CURRENT_LIST_DIR}/adcs/AdcsTransport.cpp"
)
target_compile_features(obc_adcs_transport PUBLIC cxx_std_14)
target_include_directories(obc_adcs_transport
  PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/adcs"
)
target_link_libraries(obc_adcs_transport
  PUBLIC
    obc_csp_runtime
)

add_library(obc_adcs_sim STATIC
  "${CMAKE_CURRENT_LIST_DIR}/adcs/AdcsSimModel.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/adcs/AdcsSimServer.cpp"
)
target_compile_features(obc_adcs_sim PUBLIC cxx_std_14)
target_include_directories(obc_adcs_sim
  PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/adcs"
)
target_link_libraries(obc_adcs_sim
  PUBLIC
    obc_csp_runtime
    Threads::Threads
)

add_executable(adcs_simulator
  "${CMAKE_CURRENT_LIST_DIR}/adcs/AdcsSimulatorMain.cpp"
)
target_compile_features(adcs_simulator PRIVATE cxx_std_14)
target_link_libraries(adcs_simulator PRIVATE obc_adcs_sim)

add_library(obc_comm_transport STATIC
  "${CMAKE_CURRENT_LIST_DIR}/comm/ByteStreamTransport.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/comm/TransparentLinkFraming.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/comm/RadioTransport.cpp"
)
target_compile_features(obc_comm_transport PUBLIC cxx_std_14)
target_include_directories(obc_comm_transport
  PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/comm"
)
target_link_libraries(obc_comm_transport
  PUBLIC
    Threads::Threads
)

add_library(obc_comm_groundlink STATIC
  "${CMAKE_CURRENT_LIST_DIR}/comm/StreamIo.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/comm/GroundLinkBackend.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/comm/CommSimModel.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/comm/SerialIngressAcquisitionFilter.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/comm/CommNodeServer.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/comm/GroundGatewayProxy.cpp"
)
target_compile_features(obc_comm_groundlink PUBLIC cxx_std_14)
target_include_directories(obc_comm_groundlink
  PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/comm"
    "${CMAKE_BINARY_DIR}/F-Prime/default"
    "${CMAKE_BINARY_DIR}/cmake/platform/unix"
)
target_link_libraries(obc_comm_groundlink
  PUBLIC
    obc_csp_runtime
    CFDP_Checksum
    Fw_Buffer
    Fw_FilePacket
    OBC_Components_CommandIngressAuthority
    Os
    Threads::Threads
)

set(OBC_COMM_GROUNDLINK_PROVIDER_LIBS
  Fw_StringFormat_snprintf
  Os_File_Posix
)

add_executable(comm_csp_node
  "${CMAKE_CURRENT_LIST_DIR}/comm/CommNodeMain.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/comm/CommNodeApp.cpp"
)
target_compile_features(comm_csp_node PRIVATE cxx_std_14)
target_link_libraries(
  comm_csp_node
  PRIVATE
    obc_comm_groundlink
    Threads::Threads
    ${OBC_COMM_GROUNDLINK_PROVIDER_LIBS}
)

add_executable(sband_comm_csp_node
  "${CMAKE_CURRENT_LIST_DIR}/comm/SbandCommNodeMain.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/comm/CommNodeApp.cpp"
)
target_compile_features(sband_comm_csp_node PRIVATE cxx_std_14)
target_link_libraries(
  sband_comm_csp_node
  PRIVATE
    obc_comm_groundlink
    Threads::Threads
    ${OBC_COMM_GROUNDLINK_PROVIDER_LIBS}
)

add_executable(uhf_comm_csp_node
  "${CMAKE_CURRENT_LIST_DIR}/comm/UhfCommNodeMain.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/comm/CommNodeApp.cpp"
)
target_compile_features(uhf_comm_csp_node PRIVATE cxx_std_14)
target_link_libraries(
  uhf_comm_csp_node
  PRIVATE
    obc_comm_groundlink
    Threads::Threads
    ${OBC_COMM_GROUNDLINK_PROVIDER_LIBS}
)

add_executable(comm_service_probe
  "${CMAKE_CURRENT_LIST_DIR}/comm/CommServiceProbeMain.cpp"
)
target_compile_features(comm_service_probe PRIVATE cxx_std_14)
target_link_libraries(
  comm_service_probe
  PRIVATE
    obc_comm_groundlink
    Threads::Threads
    ${OBC_COMM_GROUNDLINK_PROVIDER_LIBS}
)

add_executable(comm_downlink_v2_status_probe
  "${CMAKE_CURRENT_LIST_DIR}/comm/CommDownlinkV2StatusProbeMain.cpp"
)
target_compile_features(comm_downlink_v2_status_probe PRIVATE cxx_std_14)
target_link_libraries(
  comm_downlink_v2_status_probe
  PRIVATE
    obc_comm_groundlink
    Threads::Threads
    ${OBC_COMM_GROUNDLINK_PROVIDER_LIBS}
)

add_executable(comm_downlink_v3_status_probe
  "${CMAKE_CURRENT_LIST_DIR}/comm/CommDownlinkV3StatusProbeMain.cpp"
)
target_compile_features(comm_downlink_v3_status_probe PRIVATE cxx_std_14)
target_link_libraries(
  comm_downlink_v3_status_probe
  PRIVATE
    obc_comm_groundlink
    Threads::Threads
    ${OBC_COMM_GROUNDLINK_PROVIDER_LIBS}
)

add_executable(comm_downlink_v2_sender
  "${CMAKE_CURRENT_LIST_DIR}/comm/CommDownlinkV2SenderMain.cpp"
)
target_compile_features(comm_downlink_v2_sender PRIVATE cxx_std_14)
target_link_libraries(
  comm_downlink_v2_sender
  PRIVATE
    obc_comm_groundlink
    Threads::Threads
    ${OBC_COMM_GROUNDLINK_PROVIDER_LIBS}
)

add_executable(comm_downlink_v3_sender
  "${CMAKE_CURRENT_LIST_DIR}/comm/CommDownlinkV3SenderMain.cpp"
)
target_compile_features(comm_downlink_v3_sender PRIVATE cxx_std_14)
target_link_libraries(
  comm_downlink_v3_sender
  PRIVATE
    obc_comm_groundlink
    Threads::Threads
    ${OBC_COMM_GROUNDLINK_PROVIDER_LIBS}
)

add_executable(ground_ttc_gateway
  "${CMAKE_CURRENT_LIST_DIR}/comm/GroundGatewayMain.cpp"
)
target_compile_features(ground_ttc_gateway PRIVATE cxx_std_14)
target_link_libraries(
  ground_ttc_gateway
  PRIVATE
    obc_comm_groundlink
    Threads::Threads
    ${OBC_COMM_GROUNDLINK_PROVIDER_LIBS}
)

add_executable(pty_pair_bridge
  "${CMAKE_CURRENT_LIST_DIR}/comm/PtyPairBridgeMain.cpp"
)
target_compile_features(pty_pair_bridge PRIVATE cxx_std_14)
target_link_libraries(pty_pair_bridge PRIVATE Threads::Threads)

add_executable(serial_link_probe
  "${CMAKE_CURRENT_LIST_DIR}/comm/SerialLinkProbeMain.cpp"
)
target_compile_features(serial_link_probe PRIVATE cxx_std_14)
target_link_libraries(serial_link_probe PRIVATE obc_comm_transport Threads::Threads)

add_library(obc_gps_support STATIC
  "${CMAKE_CURRENT_LIST_DIR}/gps/NmeaParser.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/gps/GpsSource.cpp"
)
target_compile_features(obc_gps_support PUBLIC cxx_std_14)
target_include_directories(obc_gps_support
  PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/gps"
)

add_executable(radio_mock_server
  "${CMAKE_CURRENT_LIST_DIR}/comm/MockRadioServerMain.cpp"
)
target_compile_features(radio_mock_server PRIVATE cxx_std_14)
target_link_libraries(radio_mock_server PRIVATE obc_comm_transport Threads::Threads)

add_library(obc_scenario_replay STATIC
  "${CMAKE_CURRENT_LIST_DIR}/scenario/ScenarioTimeline.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/scenario/ScenarioBridge.cpp"
)
target_compile_features(obc_scenario_replay PUBLIC cxx_std_14)
target_include_directories(obc_scenario_replay
  PUBLIC
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/scenario"
)
target_link_libraries(obc_scenario_replay
  PUBLIC
    obc_eps_sim
    obc_adcs_sim
)

if (BUILD_TESTING)
  add_test(NAME csp_runtime_integration_test
    COMMAND bash "${PROJECT_SOURCE_DIR}/scripts/run_csp_runtime_smoke.sh"
  )
  set_tests_properties(csp_runtime_integration_test PROPERTIES TIMEOUT 60)
  if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_test(NAME csp_runtime_socketcan_missing_device_test
      COMMAND csp_runtime_smoke --transport socketcan
    )
    set_tests_properties(csp_runtime_socketcan_missing_device_test PROPERTIES WILL_FAIL TRUE)

    add_test(NAME csp_runtime_socketcan_invalid_device_test
      COMMAND csp_runtime_smoke --transport socketcan --can-device obc_missing_can_device
    )
    set_tests_properties(csp_runtime_socketcan_invalid_device_test PROPERTIES WILL_FAIL TRUE)

    add_test(NAME csp_runtime_socketcan_noncan_device_test
      COMMAND csp_runtime_smoke --transport socketcan --can-device lo
    )
    set_tests_properties(csp_runtime_socketcan_noncan_device_test PROPERTIES WILL_FAIL TRUE)
  endif()

  add_executable(eps_csp_integration_client
    "${CMAKE_CURRENT_LIST_DIR}/eps/test/EpsCspIntegrationClient.cpp"
  )
  target_compile_features(eps_csp_integration_client PRIVATE cxx_std_14)
  target_link_libraries(eps_csp_integration_client
    PRIVATE
      obc_eps_transport
      Threads::Threads
  )
  add_test(NAME eps_csp_integration_test
    COMMAND bash "${PROJECT_SOURCE_DIR}/scripts/run_eps_csp_integration.sh"
  )
  set_tests_properties(eps_csp_integration_test PROPERTIES TIMEOUT 60)

  add_executable(adcs_csp_integration_client
    "${CMAKE_CURRENT_LIST_DIR}/adcs/test/AdcsCspIntegrationClient.cpp"
  )
  target_compile_features(adcs_csp_integration_client PRIVATE cxx_std_14)
  target_link_libraries(adcs_csp_integration_client
    PRIVATE
      obc_adcs_transport
      Threads::Threads
  )
  add_test(NAME adcs_csp_integration_test
    COMMAND bash "${PROJECT_SOURCE_DIR}/scripts/run_adcs_csp_integration.sh"
  )
  set_tests_properties(adcs_csp_integration_test PROPERTIES TIMEOUT 60)

  add_executable(adcs_sim_model_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/adcs/test/AdcsSimModelUnitTest.cpp"
  )
  target_compile_features(adcs_sim_model_unit_test PRIVATE cxx_std_14)
  target_link_libraries(adcs_sim_model_unit_test
    PRIVATE
      obc_adcs_sim
  )
  add_test(NAME adcs_sim_model_unit_test COMMAND adcs_sim_model_unit_test)

  add_executable(adcs_sim_control_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/adcs/test/AdcsSimControlUnitTest.cpp"
  )
  target_compile_features(adcs_sim_control_unit_test PRIVATE cxx_std_14)
  target_link_libraries(adcs_sim_control_unit_test
    PRIVATE
      obc_adcs_sim
      Threads::Threads
  )
  add_test(NAME adcs_sim_control_unit_test COMMAND adcs_sim_control_unit_test)

  add_executable(eps_sim_model_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/eps/test/EpsSimModelUnitTest.cpp"
  )
  target_compile_features(eps_sim_model_unit_test PRIVATE cxx_std_14)
  target_link_libraries(eps_sim_model_unit_test
    PRIVATE
      obc_eps_sim
  )
  add_test(NAME eps_sim_model_unit_test COMMAND eps_sim_model_unit_test)

  add_executable(comm_transport_integration_test
    "${CMAKE_CURRENT_LIST_DIR}/comm/test/CommTransportIntegrationTest.cpp"
  )
  target_compile_features(comm_transport_integration_test PRIVATE cxx_std_14)
  target_link_libraries(comm_transport_integration_test
    PRIVATE
      obc_comm_transport
      Threads::Threads
  )
  add_test(NAME comm_transport_integration_test COMMAND comm_transport_integration_test)

  add_executable(gps_support_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/gps/test/GpsSupportUnitTest.cpp"
  )
  target_compile_features(gps_support_unit_test PRIVATE cxx_std_14)
  target_link_libraries(gps_support_unit_test
    PRIVATE
      obc_gps_support
  )
  add_test(NAME gps_support_unit_test COMMAND gps_support_unit_test)

  add_executable(transparent_link_framing_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/comm/test/TransparentLinkFramingUnitTest.cpp"
  )
  target_compile_features(transparent_link_framing_unit_test PRIVATE cxx_std_14)
  target_link_libraries(transparent_link_framing_unit_test
    PRIVATE
      obc_comm_transport
  )
  add_test(NAME transparent_link_framing_unit_test COMMAND transparent_link_framing_unit_test)

  add_executable(comm_groundlink_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/comm/test/CommGroundLinkUnitTest.cpp"
  )
  target_compile_features(comm_groundlink_unit_test PRIVATE cxx_std_14)
  target_link_libraries(comm_groundlink_unit_test
    PRIVATE
      obc_comm_groundlink
      Threads::Threads
      ${OBC_COMM_GROUNDLINK_PROVIDER_LIBS}
  )
  add_test(NAME comm_groundlink_unit_test COMMAND comm_groundlink_unit_test)

  add_executable(comm_sim_model_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/comm/test/CommSimModelUnitTest.cpp"
  )
  target_compile_features(comm_sim_model_unit_test PRIVATE cxx_std_14)
  target_link_libraries(comm_sim_model_unit_test
    PRIVATE
      obc_comm_groundlink
  )
  add_test(NAME comm_sim_model_unit_test COMMAND comm_sim_model_unit_test)

  add_executable(serial_ingress_filter_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/comm/test/SerialIngressFilterUnitTest.cpp"
  )
  target_compile_features(serial_ingress_filter_unit_test PRIVATE cxx_std_14)
  target_link_libraries(serial_ingress_filter_unit_test
    PRIVATE
      obc_comm_groundlink
  )
  add_test(NAME serial_ingress_filter_unit_test COMMAND serial_ingress_filter_unit_test)

  add_executable(scenario_bridge_integration_test
    "${CMAKE_CURRENT_LIST_DIR}/scenario/test/ScenarioBridgeIntegrationTest.cpp"
  )
  target_compile_features(scenario_bridge_integration_test PRIVATE cxx_std_14)
  target_link_libraries(scenario_bridge_integration_test
    PRIVATE
      obc_scenario_replay
  )
  add_test(NAME scenario_bridge_integration_test COMMAND scenario_bridge_integration_test)

  add_executable(eps_sim_control_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/eps/test/EpsSimControlUnitTest.cpp"
  )
  target_compile_features(eps_sim_control_unit_test PRIVATE cxx_std_14)
  target_link_libraries(eps_sim_control_unit_test
    PRIVATE
      obc_eps_sim
  )
  add_test(NAME eps_sim_control_unit_test COMMAND eps_sim_control_unit_test)
endif()
