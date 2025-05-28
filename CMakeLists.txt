cmake_policy(SET CMP0048 NEW) # control VERSION variables
cmake_minimum_required(VERSION 3.7)
project(HPWHsim LANGUAGES CXX)
cmake_policy(SET CMP0079 NEW) # target_link_libraries() allows use with targets in other directories.
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0076 NEW)

# Set a default build type if none was specified
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
            "MinSizeRel" "RelWithDebInfo")
endif ()

find_package(Git QUIET)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CMakeDependentOption)

cmake_dependent_option(${PROJECT_NAME}_BUILD_TESTING "Build ${PROJECT_NAME} testing targets" ON "${PROJECT_NAME}_IS_TOP_LEVEL" OFF)
option(${PROJECT_NAME}_COVERAGE "Add ${PROJECT_NAME} coverage reports" OFF)
#cmake_dependent_option(${PROJECT_NAME}_BUILD_EXAMPLES "Build ${PROJECT_NAME} examples" ON "${PROJECT_NAME}_IS_TOP_LEVEL" OFF)
cmake_dependent_option(${PROJECT_NAME}_WARNINGS_AS_ERRORS "Treat warnings in ${PROJECT_NAME} as errors" ON "${PROJECT_NAME}_IS_TOP_LEVEL" OFF)

if (HPWHSIM_OMIT_TESTTOOL)
    set_property(GLOBAL PROPERTY ${PROJECT_NAME}_BUILD_TESTING OFF)
endif ()

if (HPWHSIM_ABRIDGED)
    add_compile_definitions(HPWH_ABRIDGED)
endif ()

if (NOT ${PROJECT_NAME}_STATIC_LIB)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()

include(compiler-flags)

# Set up testing/coverage
if (${PROJECT_NAME}_BUILD_TESTING)
    enable_testing()
    if (${PROJECT_NAME}_COVERAGE)
        set(ENABLE_COVERAGE ON CACHE BOOL " " FORCE)
        find_package(codecov)
    endif ()
endif ()

add_subdirectory(vendor)

#generate presets
set(srcCodeDir "${PROJECT_BINARY_DIR}/presets")
set(pythonScriptDir "${PROJECT_SOURCE_DIR}/scripts/python/data_model")

include(poetry-install)
poetry_install(${pythonScriptDir})
execute_process(
        COMMAND poetry run python "incorp_presets.py" "JSON" "${PROJECT_BINARY_DIR}" "IHPWH_models.json" "CWHS_models.json"
        WORKING_DIRECTORY "${pythonScriptDir}"
        RESULT_VARIABLE build_presets_result)

if (build_presets_result EQUAL 0)
     message(STATUS "Preset generation succeeded")
else()
    message(STATUS "Preset generation failed")
endif()

add_custom_target(generate_presets ALL DEPENDS "${PROJECT_BINARY_DIR}/presets")
if(${CMAKE_PROJECT_NAME} STREQUAL "HPWHsim")
    add_subdirectory("${PROJECT_BINARY_DIR}/presets")
endif()

#
add_subdirectory(src)

if (${PROJECT_NAME}_BUILD_TESTING)
	add_subdirectory(test)
    if (${PROJECT_NAME}_COVERAGE)
        coverage_evaluate()
    endif ()
endif ()
