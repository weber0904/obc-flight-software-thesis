####
# EpsBridge module and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/EpsBridge.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/EpsBridge.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/EpsBridge.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_EpsFdirController
    OBC_Components_ModeSafetyController
)
target_link_libraries(OBC_Components_EpsBridge PRIVATE obc_eps_transport)
target_include_directories(OBC_Components_EpsBridge PRIVATE
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/simulators/common"
  "${PROJECT_SOURCE_DIR}/simulators/eps"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/EpsBridgeTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/EpsBridgeTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/EpsBridge.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/EpsBridgeTester.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_EpsFdirController
    OBC_Components_ModeSafetyController
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_EpsBridge_ut_exe)
  target_link_libraries(OBC_Components_EpsBridge_ut_exe PRIVATE obc_eps_transport)
  target_include_directories(OBC_Components_EpsBridge_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/common"
    "${PROJECT_SOURCE_DIR}/simulators/eps"
  )
endif()
