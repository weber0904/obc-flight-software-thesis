####
# CspBridge module and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/CspBridge.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CspBridge.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/CspBridge.hpp"
  DEPENDS
    OBC_Types
)
target_link_libraries(OBC_Components_CspBridge PRIVATE obc_csp_runtime)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CspBridgeTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CspBridgeTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CspBridge.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CspBridgeTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_CspBridge_ut_exe)
  target_link_libraries(OBC_Components_CspBridge_ut_exe PRIVATE obc_csp_runtime)
endif()
