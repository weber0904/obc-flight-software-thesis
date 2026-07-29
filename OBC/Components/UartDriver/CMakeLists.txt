####
# UartDriver module and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/UartDriver.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/UartDriver.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/UartDriver.hpp"
  DEPENDS
    OBC_Types
)
target_link_libraries(OBC_Components_UartDriver PRIVATE obc_comm_transport)
target_include_directories(OBC_Components_UartDriver PRIVATE
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/simulators/comm"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/UartDriverTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/UartDriverTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/UartDriver.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/UartDriverTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_UartDriver_ut_exe)
  target_link_libraries(OBC_Components_UartDriver_ut_exe PRIVATE obc_comm_transport)
  target_include_directories(OBC_Components_UartDriver_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/comm"
  )
endif()
