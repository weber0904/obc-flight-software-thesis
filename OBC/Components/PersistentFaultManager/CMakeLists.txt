####
# PersistentFaultManager component and focused tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/PersistentFaultManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PersistentFaultStore.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/PersistentFaultManager.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/PersistentFaultManager.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/PersistentFaultRuntime.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/PersistentFaultStore.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_OnboardStateData
    Os
)
target_include_directories(OBC_Components_PersistentFaultManager PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/PersistentFaultManagerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/PersistentFaultManagerTestMain.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/PersistentFaultManager.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/PersistentFaultManagerTester.hpp"
  DEPENDS
    OBC_Types
    OBC_Components_OnboardStateData
    Os
  UT_AUTO_HELPERS
)
if (TARGET OBC_Components_PersistentFaultManager_ut_exe)
  target_include_directories(OBC_Components_PersistentFaultManager_ut_exe PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(OBC_Components_PersistentFaultManager_ut_exe
    PRIVATE
      Os_File
      Os_File_Posix
  )
endif()

if (BUILD_TESTING)
  add_executable(persistent_fault_store_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/PersistentFaultStore.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/PersistentFaultStoreUnitTest.cpp"
  )
  target_compile_features(persistent_fault_store_unit_test PRIVATE cxx_std_14)
  target_include_directories(persistent_fault_store_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(persistent_fault_store_unit_test
    PRIVATE
      OBC_Types
      OBC_Components_OnboardStateData
      Os
      Os_File
      Os_File_Posix
      Fw_StringFormat_snprintf
  )
  add_test(NAME persistent_fault_store_unit_test COMMAND persistent_fault_store_unit_test)
endif()
