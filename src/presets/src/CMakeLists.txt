
file(GLOB headers "include/*.h")
file(GLOB sources "src/*.cpp")

file(GLOB lib_headers "${PROJECT_SOURCE_DIR}/include/*.h")
file(GLOB lib_src "${PROJECT_SOURCE_DIR}/src/*.cpp")

set (sources "${lib_headers}"
             "${lib_src}")

add_library(hpwh_presets STATIC ${sources})

target_include_directories(hpwh_presets PUBLIC ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/include)

target_compile_features(hpwh_presets PRIVATE cxx_std_17)
include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})