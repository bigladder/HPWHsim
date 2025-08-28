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

set(presets_directory "${PROJECT_BINARY_DIR}/presets")

file(GLOB presets_headers "${presets_directory}/include/*.h")

set(headers
        HPWHversion.in.hh
        HPWH.hh
        HPWHUtils.hh
        HPWHHeatSource.hh
        HPWHHeatingLogic.hh
        Tank.hh
        Condenser.hh
        Resistance.hh
        hpwh-data-model.hh
        HPWHFitter.hh
        "${presets_directory}/presets.h"
        "${presets_headers}"
        )

set(sources
        HPWHversion.cc
        HPWH.cc
        HPWHUtils.cc
        HPWHHeatSource.cc
        HPWHHeatingLogic.cc
        HPWH.cc
        HPWHpresets.cc
        Tank.cc
        Condenser.cc
        Resistance.cc
        hpwh-data-model.cpp
        HPWHFitter.cc
        "${presets_directory}/presets.cpp"
        )

set(library_sources
        ${sources}
        ${headers}
        )

set_source_files_properties(
        "${presets_directory}/presets.cpp"
        "${presets_directory}/presets.h"
        PROPERTIES GENERATED TRUE
)
option(${PROJECT_NAME}_STATIC_LIB "Make ${PROJECT_NAME} a static library" ON)

if (${PROJECT_NAME}_STATIC_LIB)
    add_library(${PROJECT_NAME} STATIC ${library_sources})
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D${PROJECT_NAME}_STATIC_DEFINE")
else ()
    set(CMAKE_MACOSX_RPATH 1)
    add_library(${PROJECT_NAME} SHARED ${library_sources})
endif ()

target_include_directories(${PROJECT_NAME}
        PUBLIC ${PROJECT_BINARY_DIR}/src
        PUBLIC ${PROJECT_SOURCE_DIR}/src
        PUBLIC "${presets_directory}"
        PUBLIC "${presets_directory}/include"
        )

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_common_interface PUBLIC btwxt nlohmann_json::nlohmann_json hpwh_data_model)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})

add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_version_header generate_hpwhsim_presets)

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
