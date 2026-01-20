# SPDX-FileCopyrightText: © 2026 Big Ladder Software <info@bigladdersoftware.com>
# SPDX-License-Identifier: BSD-3-Clause
# See the LICENSE.txt file for additional terms and conditions.
# NOTE: THIS FILE IS AUTO-GENERATED.

file(GLOB lib_headers "${PROJECT_SOURCE_DIR}/include/hpwh_data_model/*.h")
file(GLOB lib_src "${PROJECT_SOURCE_DIR}/src/*.cpp")

set (library_sources "${lib_headers}"
                     "${lib_src}")

option(${PROJECT_NAME}_STATIC_LIB "Make ${PROJECT_NAME} a static library" ON)

if (${PROJECT_NAME}_STATIC_LIB)
    add_library(${PROJECT_NAME} STATIC ${library_sources})
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D${PROJECT_NAME}_STATIC_DEFINE")
else ()
    set(CMAKE_MACOSX_RPATH 1)
    add_library(${PROJECT_NAME} SHARED ${library_sources})
endif ()

target_include_directories(${PROJECT_NAME}
    PUBLIC "${PROJECT_SOURCE_DIR}/include"
    PUBLIC "${PROJECT_BINARY_DIR}/include"
    PRIVATE "${PROJECT_SOURCE_DIR}/src"
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_link_libraries(hpwh_data_model PUBLIC nlohmann_json)
target_link_libraries(hpwh_data_model PUBLIC courier)
target_link_libraries(hpwh_data_model PUBLIC fmt)

target_include_directories(hpwh_data_model PUBLIC ${PROJECT_SOURCE_DIR}/include/hpwh_data_model)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_build_options)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})

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
