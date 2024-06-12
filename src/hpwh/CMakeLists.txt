# Build hpwh.exe
set(source_files
        main.cpp
        runCommand.cpp
        makeCommand.cpp
        measureCommand.cpp
        )

add_executable(hpwh ${source_files})

target_include_directories(hpwh
        PRIVATE "${PROJECT_SOURCE_DIR}/vendor/CLI11")

set(libs ${PROJECT_NAME} ${PROJECT_NAME}_common_interface CLI11)
target_link_libraries(hpwh PRIVATE ${libs})

target_compile_features(hpwh PRIVATE cxx_std_17)
