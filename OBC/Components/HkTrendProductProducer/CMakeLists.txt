
register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/HkTrendProductProducer.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/HkTrendProductProducer.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/HkTrendProductProducer.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_OnboardStateData
    Fw_Dp
)
target_include_directories(OBC_Components_HkTrendProductProducer PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/HkTrendProductProducerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/HkTrendProductProducerTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/HkTrendProductProducer.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/HkTrendProductProducerTester.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_OnboardStateData
    Fw_Dp
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_HkTrendProductProducer_ut_exe)
  target_include_directories(OBC_Components_HkTrendProductProducer_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()
