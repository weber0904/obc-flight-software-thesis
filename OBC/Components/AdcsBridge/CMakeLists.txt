####
# AdcsBridge module and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/AdcsBridge.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/AdcsBridge.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/AdcsBridge.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/AdcsRuntime.hpp"
  DEPENDS
    OBC_Types
)
target_link_libraries(OBC_Components_AdcsBridge PRIVATE obc_adcs_transport)
target_include_directories(OBC_Components_AdcsBridge PRIVATE
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/simulators/common"
  "${PROJECT_SOURCE_DIR}/simulators/adcs"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/AdcsBridgeTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/AdcsBridgeTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/AdcsBridge.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/AdcsBridgeTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_AdcsBridge_ut_exe)
  target_link_libraries(OBC_Components_AdcsBridge_ut_exe PRIVATE obc_adcs_transport)
  target_include_directories(OBC_Components_AdcsBridge_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/common"
    "${PROJECT_SOURCE_DIR}/simulators/adcs"
  )
endif()
