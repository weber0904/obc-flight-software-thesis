####
# RadioController module and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/RadioController.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/RadioController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/RadioController.hpp"
  DEPENDS
    OBC_Types
)
target_link_libraries(OBC_Components_RadioController PRIVATE obc_comm_transport)
target_include_directories(OBC_Components_RadioController PRIVATE
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/simulators/comm"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/RadioControllerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/RadioControllerTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/RadioController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/RadioControllerTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_RadioController_ut_exe)
  target_link_libraries(OBC_Components_RadioController_ut_exe PRIVATE obc_comm_transport)
  target_include_directories(OBC_Components_RadioController_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/simulators/comm"
  )
endif()
