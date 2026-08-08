####
# GroundLinkHealthProvider component and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/GroundLinkHealthProvider.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/GroundLinkHealthProvider.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/GroundLinkHealthProvider.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/GroundLinkHealthRuntime.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_GroundLinkDriver
)
target_link_libraries(OBC_Components_GroundLinkHealthProvider PRIVATE obc_comm_groundlink)
target_include_directories(OBC_Components_GroundLinkHealthProvider PRIVATE
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/simulators/comm"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/GroundLinkHealthProviderTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/GroundLinkHealthProviderTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/GroundLinkHealthProvider.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/GroundLinkHealthProviderTester.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_GroundLinkDriver
  UT_AUTO_HELPERS
)

if (TARGET OBC_Components_GroundLinkHealthProvider_ut_exe)
  target_link_libraries(OBC_Components_GroundLinkHealthProvider_ut_exe PRIVATE obc_comm_groundlink)
  target_include_directories(OBC_Components_GroundLinkHealthProvider_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/comm"
  )
endif()
