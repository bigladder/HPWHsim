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
add_subdirectory(src)

# Either generate hpwh-data-model code, or use existing code
set(${PROJECT_NAME}_USE_DATA_MODEL_GENERATION TRUE CACHE BOOL "Use data-model source code generation" FORCE)
if (${PROJECT_NAME}_USE_DATA_MODEL_GENERATION)
    set(genOutDir "${PROJECT_BINARY_DIR}/hpwh_data_model")
    if (NOT ${PROJECT_NAME}_DATA_MODEL_AVAILABLE)
        set(pythonScriptDir "${PROJECT_SOURCE_DIR}/scripts/python/data_model")
        set(dataModelDir "${PROJECT_SOURCE_DIR}/vendor/hpwh_data_model")

        include(poetry-install)
        poetry_install(${pythonScriptDir})

        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init "${dataModelDir}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}

            COMMAND poetry run python "build_data_model.py" "${PROJECT_SOURCE_DIR}" "${dataModelDir}" "${genOutDir}"
            WORKING_DIRECTORY "${pythonScriptDir}")

        set(${PROJECT_NAME}_DATA_MODEL_AVAILABLE TRUE CACHE BOOL "Python data-model source code is available" FORCE)
    endif()
else()
    set(genOutDir "${PROJECT_SOURCE_DIR}/hpwh_data_model")
    set(${PROJECT_NAME}_DATA_MODEL_AVAILABLE TRUE CACHE BOOL "Python data-model source code is available" FORCE)
endif()

if (${PROJECT_NAME}_DATA_MODEL_AVAILABLE)
	add_subdirectory("${genOutDir}/cpp")

	set(gen_include
		ASHRAE205.h, 
		load-object.h,
		HPWHSimInput.h,
		HeatSourceConfiguration.h,
		RSRESISTANCEWATERHEATSOURCE.h,
		RSCONDENSERWATERHEATSOURCE.h,
		RSINTEGRATEDWATERHEATER.h,
		RSAIRTOWATERHEATPUMP.h,
		CentralWaterHeatingSystem.h,
		RSTANK.h)
    list(TRANSFORM gen_include PREPEND "${genOutDir}/cpp/include/hpwh_data_model/")

    set(gen_src
        ASHRAE205.cpp,
        HPWHSimInput.cpp,
        HeatSourceConfiguration.cpp,
        RSRESISTANCEWATERHEATSOURCE.cpp,
        RSCONDENSERWATERHEATSOURCE.cpp,
        RSINTEGRATEDWATERHEATER.cpp,
        RSAIRTOWATERHEATPUMP.cpp,
        CentralWaterHeatingSystem.cpp,
        RSTANK.cpp,
        core.cpp)
    list(TRANSFORM gen_src PREPEND "${genOutDir}/cpp/src/")

    set(gen_files gen_include gen_src "${genOutDir}/cpp/CMakeLists.txt" "${genOutDir}/cpp/src/CMakeLists.txt")
	add_custom_target(generate_data_model ALL DEPENDS gen_files)
endif ()

if (${PROJECT_NAME}_BUILD_TESTING)
	add_subdirectory(test)
    if (${PROJECT_NAME}_COVERAGE)
        coverage_evaluate()
    endif ()
endif ()
