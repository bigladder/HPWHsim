set(library_files
        unit-test.hh
        scaleTest.cpp
        energyBalanceTest.cpp
        fixedTankSizeTest.cpp
        sizingFractionsTest.cpp
        maxSetpointTest.cpp
        heatingLogicsTest.cpp
        stateOfChargeFncsTest.cpp
        resistanceFncsTest.cpp
        compressorFncsTest.cpp
        performanceMapTest.cpp
        measureMetricsTest.cpp
        unitsConversionTest.cpp
        unit-test-main.cpp
        )

add_executable(${PROJECT_NAME}_tests ${library_files})

target_compile_features(${PROJECT_NAME}_tests PRIVATE cxx_std_17)

target_include_directories(${PROJECT_NAME}_tests PRIVATE ${PROJECT_BINARY_DIR}/src "${PROJECT_SOURCE_DIR}/src")

target_link_libraries(${PROJECT_NAME}_tests ${PROJECT_NAME} gtest gmock fmt)

include(GoogleTest)

gtest_discover_tests(
        ${PROJECT_NAME}_tests
        TEST_PREFIX ${PROJECT_NAME}:
)
