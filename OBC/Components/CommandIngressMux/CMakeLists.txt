

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/CommandIngressMux.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CommandIngressMux.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/CommandIngressMux.hpp"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommandIngressMuxTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommandIngressMuxTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CommandIngressMux.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommandIngressMuxTester.hpp"
  UT_AUTO_HELPERS
)
