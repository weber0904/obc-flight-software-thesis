
register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/BeaconPublisher.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/BeaconPublisher.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/BeaconPublisher.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_OnboardStateData
)
target_include_directories(OBC_Components_BeaconPublisher PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/BeaconPublisherTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/BeaconPublisherTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/BeaconPublisher.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/BeaconPublisherTester.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_OnboardStateData
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_BeaconPublisher_ut_exe)
  target_include_directories(OBC_Components_BeaconPublisher_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()
