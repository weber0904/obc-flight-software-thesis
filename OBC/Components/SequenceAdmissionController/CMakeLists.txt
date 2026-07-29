
register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/SequenceAdmissionController.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SequenceFileInspector.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/SequenceAdmissionController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/SequenceAdmissionController.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/SequenceFileInspector.hpp"
  DEPENDS
    OBC_Components_CommandIngressAuthority
    OBC_Components_SequenceControlProtocol
    Svc_CmdSequencer
    Svc_SeqDispatcher
)
target_include_directories(OBC_Components_SequenceAdmissionController PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SequenceAdmissionControllerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SequenceAdmissionControllerTestMain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/SequenceFileInspectorTest.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/SequenceAdmissionController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SequenceAdmissionControllerTester.hpp"
  DEPENDS
    OBC_Components_CommandIngressAuthority
    OBC_Components_SequenceControlProtocol
    Svc_CmdSequencer
    Svc_SeqDispatcher
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_SequenceAdmissionController_ut_exe)
  target_include_directories(OBC_Components_SequenceAdmissionController_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()
