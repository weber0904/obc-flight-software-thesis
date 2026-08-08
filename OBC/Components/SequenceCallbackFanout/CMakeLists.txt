
register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/SequenceCallbackFanout.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/SequenceCallbackFanout.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/SequenceCallbackFanout.hpp"
  DEPENDS
    Svc_Seq
)
target_include_directories(OBC_Components_SequenceCallbackFanout PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SequenceCallbackFanoutTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SequenceCallbackFanoutTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/SequenceCallbackFanout.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SequenceCallbackFanoutTester.hpp"
  DEPENDS
    Svc_Seq
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_SequenceCallbackFanout_ut_exe)
  target_include_directories(OBC_Components_SequenceCallbackFanout_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()
