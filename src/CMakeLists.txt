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

set(headers
        HPWHversion.in.hh
        HPWHsim.hh
        HPWHLogger.hh
        HPWH.hh
        HPWHTank.hh
        HPWHHeatSource.hh
        HPWHHeatingLogic.hh
        )

set(sources
        HPWHversion.cc
        HPWHUtils.cc
        HPWHTank.cc
        HPWHHeatSource.cc
        HPWHHeatingLogic.cc
        HPWH.cc
        HPWHpresets.cc
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

add_subdirectory(data_model)

target_include_directories(${PROJECT_NAME}
        PUBLIC ${PROJECT_BINARY_DIR}/src
        PUBLIC ${PROJECT_SOURCE_DIR}/src
        PUBLIC ${PROJECT_SOURCE_DIR}/src/data_model
        )

#find_package(nlohmann_json 3.11.3 REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_common_interface PUBLIC btwxt nlohmann_json::nlohmann_json)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})


add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_version_header)

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

