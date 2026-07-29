
register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/FileIngressAuthority.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/FileIngressPolicy.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/FileIngressAuthority.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/FileIngressAuthority.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/FileIngressPolicy.hpp"
  DEPENDS
    Fw_FilePacket
    OBC_Components_SecureLinkAuthProtocol
    OBC_Components_FileIngressControlProtocol
)
target_include_directories(OBC_Components_FileIngressAuthority PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/FileIngressAuthorityTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/FileIngressAuthorityTestMain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/FileIngressPolicyTest.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/FileIngressAuthority.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/FileIngressAuthorityTester.hpp"
  DEPENDS
    Fw_FilePacket
    OBC_Components_SecureLinkAuthProtocol
    OBC_Components_FileIngressControlProtocol
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_FileIngressAuthority_ut_exe)
  target_include_directories(OBC_Components_FileIngressAuthority_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
endif()
