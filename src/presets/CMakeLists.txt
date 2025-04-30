cmake_minimum_required (VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(hpwh_presets)

file(GLOB model_headers "include/*.h")

set(sources "presets.h" "${model_headers}")

add_library(hpwh_presets STATIC ${sources})

#target_link_libraries(hpwh_presets PUBLIC nlohmann_json)

target_include_directories(hpwh_presets PUBLIC ${PROJECT_SOURCE_DIR}/presets/include)

target_compile_features(hpwh_presets PRIVATE cxx_std_17)
include(GenerateExportHeader)
generate_export_header(hpwh_presets)
