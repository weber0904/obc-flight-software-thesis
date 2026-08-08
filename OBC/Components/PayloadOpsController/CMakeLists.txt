####
# PayloadOpsController module and unit tests
####

register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/PayloadOpsController.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PiCameraManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/PayloadBackendHelperClient.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/PayloadOpsController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/PayloadOpsController.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/PayloadOpsRuntime.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/PiCameraManager.hpp"
  DEPENDS
    OBC_Types
)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(LIBCAMERA QUIET IMPORTED_TARGET libcamera)
endif()

add_executable(payload_camera_backend_helper
  "${CMAKE_CURRENT_LIST_DIR}/PayloadCameraBackendHelperMain.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/StubPiCameraDriver.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/LibcameraPiCameraDriver.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/third_party/toojpeg/toojpeg.cpp"
)
target_compile_features(payload_camera_backend_helper PRIVATE cxx_std_17)
target_include_directories(payload_camera_backend_helper PRIVATE
  "${PROJECT_SOURCE_DIR}"
  "${CMAKE_CURRENT_LIST_DIR}/third_party/toojpeg"
)
target_link_libraries(payload_camera_backend_helper
  PRIVATE
    OBC_Components_PayloadOpsController
    Fw_StringFormat_snprintf
)
if(LIBCAMERA_FOUND)
  target_compile_definitions(payload_camera_backend_helper PRIVATE OBC_HAS_LIBCAMERA=1)
  target_link_libraries(payload_camera_backend_helper PRIVATE PkgConfig::LIBCAMERA)
endif()

register_fprime_ut(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/PayloadOpsControllerTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/PayloadOpsControllerTestMain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/PiCameraManagerTest.cpp"
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/PayloadOpsController.fpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/PayloadOpsControllerTester.hpp"
  DEPENDS
    OBC_Types
  UT_AUTO_HELPERS
)

if (TARGET OBC_Components_PayloadOpsController_ut_exe)
  add_dependencies(OBC_Components_PayloadOpsController_ut_exe payload_camera_backend_helper)
endif()

if (BUILD_TESTING)
  add_executable(payload_backend_helper_client_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/test/PayloadBackendHelperClientTest.cpp"
  )
  target_compile_features(payload_backend_helper_client_unit_test PRIVATE cxx_std_14)
  target_include_directories(payload_backend_helper_client_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(payload_backend_helper_client_unit_test
    PRIVATE
      gtest_main
      OBC_Components_PayloadOpsController
      Fw_StringFormat_snprintf
  )
  add_dependencies(payload_backend_helper_client_unit_test payload_camera_backend_helper)
  add_test(NAME payload_backend_helper_client_unit_test COMMAND payload_backend_helper_client_unit_test)
endif()
