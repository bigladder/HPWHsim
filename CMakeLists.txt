project(HPWHsim)

cmake_minimum_required(VERSION 3.5)

enable_testing()

set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(compiler-flags)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

find_package(Git QUIET)

if (HPWHSIM_ABRIDGED)
  add_compile_definitions( HPWH_ABRIDGED)
endif()

add_subdirectory(vendor)
add_subdirectory(src)

if (NOT HPWHSIM_OMIT_TESTTOOL)
  add_subdirectory(test)
endif()
