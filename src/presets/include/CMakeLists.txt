file(GLOB model_headers "${PROJECT_SOURCE_DIR}/models/*.h")

set (sources "${PROJECT_SOURCE_DIR}/models/models.h" "${model_headers}"

option(${PROJECT_NAME}_STATIC_LIB "Make ${PROJECT_NAME} a static library" ON)

if (${PROJECT_NAME}_STATIC_LIB)
    add_library(${PROJECT_NAME} STATIC ${sources})
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D${PROJECT_NAME}_STATIC_DEFINE")
else ()
    set(CMAKE_MACOSX_RPATH 1)
    add_library(${PROJECT_NAME} SHARED ${sources})
endif ()



target_link_libraries(hpwh_data_model PUBLIC nlohmann_json)



target_link_libraries(hpwh_data_model PUBLIC courier)



target_link_libraries(hpwh_data_model PUBLIC fmt)


target_include_directories(hpwh_data_model PUBLIC ${PROJECT_SOURCE_DIR}/include/hpwh_data_model)

target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -Wpedantic>
        )

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})
