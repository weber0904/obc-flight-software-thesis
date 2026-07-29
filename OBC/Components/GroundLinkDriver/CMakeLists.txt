####
# GroundLinkDriver module and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/GroundLinkDriver.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/GroundLinkDriver.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/GroundLinkDriver.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/GroundLinkObservationRuntime.hpp"
  DEPENDS
    OBC_Types
)
target_link_libraries(OBC_Components_GroundLinkDriver PRIVATE obc_comm_groundlink)
target_include_directories(OBC_Components_GroundLinkDriver PRIVATE
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/simulators/comm"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/GroundLinkDriverTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/GroundLinkDriverTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/GroundLinkDriver.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/GroundLinkDriverTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_GroundLinkDriver_ut_exe)
  target_link_libraries(OBC_Components_GroundLinkDriver_ut_exe PRIVATE obc_comm_groundlink)
  target_include_directories(OBC_Components_GroundLinkDriver_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/comm"
  )
endif()
