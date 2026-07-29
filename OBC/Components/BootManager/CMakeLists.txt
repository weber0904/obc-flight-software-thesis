####
# BootManager module and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/BootManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/BootManifestVerifier.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/BootMetadataStore.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/BootManager.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/BootManager.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/BootManifestVerifier.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/BootMetadataStore.hpp"
  DEPENDS
    OBC_Components_CommandIngressAuthority
    OBC_Components_PersistentFaultManager
    OBC_Types
    Os
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/BootManifestVerifierTest.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/BootManagerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/BootManagerTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/BootManager.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/BootManagerTester.hpp"
  DEPENDS
    OBC_Components_CommandIngressAuthority
    OBC_Components_PersistentFaultManager
    OBC_Types
    Os
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_BootManager_ut_exe)
  target_include_directories(OBC_Components_BootManager_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(OBC_Components_BootManager_ut_exe
    PRIVATE
      Os_File
      Os_File_Posix
  )
endif()

if (BUILD_TESTING)
  add_executable(boot_metadata_store_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/BootMetadataStore.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/BootMetadataStoreUnitTest.cpp"
  )
  target_compile_features(boot_metadata_store_unit_test PRIVATE cxx_std_14)
  target_include_directories(boot_metadata_store_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(boot_metadata_store_unit_test
    PRIVATE
      OBC_Types
      Os
      Os_File
      Os_File_Posix
      Fw_StringFormat_snprintf
  )
  add_test(NAME boot_metadata_store_unit_test COMMAND boot_metadata_store_unit_test)
endif()
