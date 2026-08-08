
register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/DpCatalogFileDownlinkGate.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/DpCatalogFileDownlinkGate.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/DpCatalogFileDownlinkGate.hpp"
  DEPENDS
    Svc_FileDownlinkPorts
)
target_include_directories(OBC_Components_DpCatalogFileDownlinkGate PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/DpCatalogFileDownlinkGateTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/DpCatalogFileDownlinkGateTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/DpCatalogFileDownlinkGate.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/DpCatalogFileDownlinkGateTester.hpp"
  DEPENDS
    Svc_FileDownlinkPorts
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_DpCatalogFileDownlinkGate_ut_exe)
  target_include_directories(OBC_Components_DpCatalogFileDownlinkGate_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()
