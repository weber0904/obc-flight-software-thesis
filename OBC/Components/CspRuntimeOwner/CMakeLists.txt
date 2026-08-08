####
# CspRuntimeOwner component
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/CspRuntimeOwner.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CspRuntimeOwner.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/AsyncCspRuntimeOwner.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CspRuntimeOwner.hpp"
  DEPENDS
    OBC_Types
)
target_link_libraries(OBC_Components_CspRuntimeOwner PRIVATE obc_csp_runtime)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CspRuntimeOwnerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CspRuntimeOwnerTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CspRuntimeOwner.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CspRuntimeOwnerTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
