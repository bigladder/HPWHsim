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
        HPWH.hh
        )

set(sources
        HPWHversion.cc
        HPWH.cc
        HPWHHeatSources.cc
        HPWHHeatingLogics.cc
        HPWHpresets.cc
        )

set(library_sources
        ${sources}
        ${headers}
        )

option(${PROJECT_NAME}_STATIC_LIB "Make ${PROJECT_NAME} a static library" ON)

if (${PROJECT_NAME}_STATIC_LIB)
    add_library(libHPWHsim STATIC ${library_sources}) # TODO: Change name to ${PROJECT_NAME}
    set_target_properties(libHPWHsim PROPERTIES COMPILE_FLAGS "-D${PROJECT_NAME}_STATIC_DEFINE")
else ()
    set(CMAKE_MACOSX_RPATH 1)
    add_library(libHPWHsim SHARED ${library_sources})
endif ()

target_include_directories(libHPWHsim PUBLIC ${PROJECT_BINARY_DIR}/src ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(libHPWHsim PRIVATE ${PROJECT_NAME}_common_interface PUBLIC btwxt)
target_compile_features(libHPWHsim PRIVATE cxx_std_17)

include(GenerateExportHeader)
generate_export_header(libHPWHsim)


add_dependencies(libHPWHsim ${PROJECT_NAME}_version_header)

set_target_properties(libHPWHsim PROPERTIES OUTPUT_NAME HPWHsim)
set_target_properties(libHPWHsim PROPERTIES PDB_NAME libHPWHsim)

# If MSVC_RUNTIME_LIBRARY is not by a parent project use the default.
# It's not clear why this is needed, since documentation indicates it
#   should be happening with the CMP0091 policy set to NEW.
get_target_property(RTL libHPWHsim MSVC_RUNTIME_LIBRARY)
if ("${RTL}" STREQUAL "RTL-NOTFOUND")
    set_target_properties(libHPWHsim PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif ()

if (${PROJECT_NAME}_COVERAGE)
    add_coverage(libHPWHsim)
endif ()

