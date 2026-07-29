register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/SecureLinkAuthorizer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SecureLinkHandshake.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/SecureLinkAuthorizer.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/SecureLinkAuthorizer.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/SecureLinkHandshake.hpp"
  DEPENDS
    OBC_Components_SecureLinkAuthProtocol
    OBC_Components_CommandIngressAuthority
)
target_include_directories(OBC_Components_SecureLinkAuthorizer PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SecureLinkAuthorizerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SecureLinkAuthorizerTestMain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/SecureLinkHandshakeTest.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/SecureLinkAuthorizer.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/SecureLinkAuthorizerTester.hpp"
  DEPENDS
    OBC_Components_SecureLinkAuthProtocol
    OBC_Components_CommandIngressAuthority
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_SecureLinkAuthorizer_ut_exe)
  target_include_directories(OBC_Components_SecureLinkAuthorizer_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()
