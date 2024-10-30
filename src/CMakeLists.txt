# Set HPWHsim Version
add_custom_target(${PROJECT_NAME}_version_header
        DEPENDS ${PROJECT_SOURCE_DIR}/src/HPWHversion.in.hh
        COMMAND ${CMAKE_COMMAND}
        ARGS -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
        -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
        -DPROJECT_NAME=${PROJECT_NAME}
        -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
        -P "${PROJECT_SOURCE_DIR}/cmake/git-version.cmake"
        )

set_target_properties(${PROJECT_NAME}_version_header PROPERTIES FOLDER Dependencies/HPWHsim)

#generate hpwh-data-model source
set(pythonScriptDir "${PROJECT_SOURCE_DIR}/scripts/python")
set(dataModelDir "${PROJECT_SOURCE_DIR}/vendor/hpwh_data_model")
set(genCodeDir "${PROJECT_SOURCE_DIR}/src/hpwh_data_model")
#if (NOT ${PROJECT_NAME}_PYTHON_SETUP)
message(STATUS "Installing python packages")
execute_process(
        COMMAND poetry install
            WORKING_DIRECTORY "${pythonScriptDir}"
            RESULT_VARIABLE result0
            ERROR_VARIABLE errors0
            OUTPUT_VARIABLE outputs0

        COMMAND ${CMAKE_COMMAND} -E make_directory "${dataModelDir}"

        COMMAND ${GIT_EXECUTABLE} submodule update --init "${dataModelDir}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT

        COMMAND poetry run python "data_model/build_data_model.py" "${PROJECT_SOURCE_DIR}" "${dataModelDir}" "${genCodeDir}"
            WORKING_DIRECTORY "${pythonScriptDir}"
            RESULT_VARIABLE result1
            ERROR_VARIABLE errors1
            OUTPUT_VARIABLE outputs1)

message("Poetry install success? ${result0} ${errors0} ${outputs0}")
message("Update submodule success? ${GIT_SUBMOD_RESULT}")
message("Data-model generation success? ${result1} ${errors1} ${outputs1}")
#endif ()

set(headers
        HPWHversion.in.hh
        HPWHsim.hh
        HPWH.hh
        HPWHTank.hh
        HPWHHeatSource.hh
        HPWHHeatingLogic.hh
        hpwh-data-model.h
        )

set(sources
        HPWHversion.cc
        HPWHUtils.cc
        HPWHTank.cc
        HPWHHeatSource.cc
        HPWHHeatingLogic.cc
        HPWH.cc
        HPWHpresets.cc
        hpwh-data-model.cpp
        )

set(library_sources
        ${sources}
        ${headers}
        )

option(${PROJECT_NAME}_STATIC_LIB "Make ${PROJECT_NAME} a static library" ON)

if (${PROJECT_NAME}_STATIC_LIB)
    add_library(${PROJECT_NAME} STATIC ${library_sources}) # TODO: Change name to ${PROJECT_NAME}
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D${PROJECT_NAME}_STATIC_DEFINE")
else ()
    set(CMAKE_MACOSX_RPATH 1)
    add_library(${PROJECT_NAME} SHARED ${library_sources})
endif ()

target_include_directories(${PROJECT_NAME}
        PUBLIC ${PROJECT_BINARY_DIR}/src
        PUBLIC ${PROJECT_SOURCE_DIR}/src
        PUBLIC ${PROJECT_SOURCE_DIR}/src/hpwh_data_model
        )

#find_package(nlohmann_json 3.11.3 REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_common_interface PUBLIC btwxt nlohmann_json::nlohmann_json hpwh_data_model)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})

add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_version_header) #${PROJECT_NAME}_data_model)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES PDB_NAME ${PROJECT_NAME})

# If MSVC_RUNTIME_LIBRARY is not by a parent project use the default.
# It's not clear why this is needed, since documentation indicates it
#   should be happening with the CMP0091 policy set to NEW.
get_target_property(RTL ${PROJECT_NAME} MSVC_RUNTIME_LIBRARY)
if ("${RTL}" STREQUAL "RTL-NOTFOUND")
    set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif ()

if (${PROJECT_NAME}_COVERAGE)
    add_coverage(${PROJECT_NAME})
endif ()

if (${PROJECT_NAME}_BUILD_TESTING)
    add_subdirectory(hpwh)
endif ()

add_subdirectory(hpwh_data_model/cpp)
