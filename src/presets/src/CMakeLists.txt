
file(GLOB lib_headers "${PROJECT_SOURCE_DIR}/*.h" "${PROJECT_SOURCE_DIR}/include/*.h")
file(GLOB lib_src "${PROJECT_SOURCE_DIR}/src/*.cpp")

set (lib_files "${lib_headers}"
             "${lib_src}")

add_library(hpwh_presets STATIC ${lib_files})

target_include_directories(hpwh_presets PUBLIC ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/include)

target_compile_features(hpwh_presets PRIVATE cxx_std_17)
include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})