####
# CommandIngressAuthority component and tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/CommandIngressAuthority.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandAuthorityTypes.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandAuthorityCatalog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandSessionSequence.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandAuthCrypto.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandAuthKeystore.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandEnvelopeMetadata.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/SecureCommandV2Metadata.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CommandIngressAuthority.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/CommandIngressAuthority.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandSessionRuntimeObserver.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandAuthorityTypes.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandAuthorityCatalog.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandSessionSequence.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandAuthCrypto.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandAuthKeystore.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/CommandEnvelopeMetadata.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/SecureCommandV2Metadata.hpp"
  DEPENDS
    OBC_Components_SequenceControlProtocol
    OBC_Components_SecureLinkAuthProtocol
    OBC_Components_OnboardStateData
    Os
)
target_include_directories(OBC_Components_CommandIngressAuthority PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommandIngressAuthorityTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommandIngressAuthorityTestMain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/CommandAuthorityPolicyTest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/CommandAuthKeystoreTest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/CommandEnvelopeAuthTest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/CommandEnvelopeMetadataTest.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/CommandIngressAuthority.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/CommandIngressAuthorityTester.hpp"
  DEPENDS
    OBC_Components_SequenceControlProtocol
    OBC_Components_SecureLinkAuthProtocol
    OBC_Components_OnboardStateData
    Os
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_CommandIngressAuthority_ut_exe)
  target_include_directories(OBC_Components_CommandIngressAuthority_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(OBC_Components_CommandIngressAuthority_ut_exe
    PRIVATE
      Os_File
      Os_File_Posix
  )
endif()

if (BUILD_TESTING)
  find_program(PYTHON3_EXECUTABLE python3 REQUIRED)
  set(COMMAND_AUTHORITY_DICTIONARY_ROOT "${PROJECT_SOURCE_DIR}/build-artifacts/${CMAKE_SYSTEM_NAME}")
  add_test(
    NAME command_authority_catalog_check
    COMMAND
      "${PYTHON3_EXECUTABLE}"
      "${PROJECT_SOURCE_DIR}/scripts/generate_command_authority_catalog.py"
      --policy "${CMAKE_CURRENT_LIST_DIR}/command_authority_policy.json"
      --dictionary "${COMMAND_AUTHORITY_DICTIONARY_ROOT}/OBC/dict/AppTopologyDictionary.json"
      --output "${CMAKE_CURRENT_LIST_DIR}/CommandAuthorityCatalog.cpp"
      --check
  )
endif()
