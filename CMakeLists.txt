####
# This sets up the build system for the 'OBC' project, including
# components and deployments from project.cmake. In addition, it imports the core F Prime components.
####

cmake_minimum_required(VERSION 3.24.2)
project(OBC C CXX)

if (IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/cmake")
    list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
endif()

###
# F' Core Setup
# This includes all of the F prime core components, and imports the make-system.
###
include("${CMAKE_CURRENT_LIST_DIR}/lib/fprime/cmake/FPrime.cmake")
# NOTE: register custom targets between these two lines
fprime_setup_included_code()

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/config")

# This includes project-wide objects
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/simulators")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/OBC")

register_fprime_deployment(
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/OBC/Main.cpp"
  DEPENDS
    OBC_Runtime
    OBC_TopCcsds
)
