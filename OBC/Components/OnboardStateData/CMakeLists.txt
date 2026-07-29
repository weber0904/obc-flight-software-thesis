
register_fprime_library(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/OnboardStateData.cpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/OnboardStateData.hpp"
  DEPENDS
    OBC_Types
    Fw_StringFormat_snprintf
)
target_include_directories(OBC_Components_OnboardStateData PRIVATE
  "${PROJECT_SOURCE_DIR}"
)

if (BUILD_TESTING)
  add_executable(onboard_state_data_unit_test
    "${CMAKE_CURRENT_LIST_DIR}/OnboardStateData.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/OnboardStateDataUnitTest.cpp"
  )
  target_compile_features(onboard_state_data_unit_test PRIVATE cxx_std_14)
  target_include_directories(onboard_state_data_unit_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(onboard_state_data_unit_test
    PRIVATE
      OBC_Components_OnboardStateData
      Fw_StringFormat_snprintf
  )
  add_test(NAME onboard_state_data_unit_test COMMAND onboard_state_data_unit_test)
endif()
