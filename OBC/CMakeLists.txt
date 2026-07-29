# This CMake file is intended to register project-wide objects.
# This allows for reuse between deployments, or other projects.

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Types")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Components")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Runtime")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/TopCcsds")
