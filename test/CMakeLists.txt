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

# Add output directory for test results
add_custom_target(results_directory ALL COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/output")

add_test(NAME "Ready.Output.Files" COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_CURRENT_BINARY_DIR}/output/DHW_YRLY.csv")

# Run model tests and unit tests on models
set(testNames
        test30
        test50
        test70
        test95
        testLockout
        testSandenCombi
        testDr_LO
        testDr_TOO
        testDr_TOT
        testDr_TOO2
        testDR_TOTLOR
        )
set(modelNames
        #  StorageTank
        AOSmithHPTS50
        AOSmithPHPT60
        AOSmithHPTU80
        Sanden80
        RheemHB50
        Stiebel220e
        GE502014
        Rheem2020Prem40
        Rheem2020Prem50
        Rheem2020Build50
        #  RheemPlugInDedicated50
        )
set(lockoutTestModels
        AOSmithPHPT60
        AOSmithHPTU80
        RheemHB50
        Stiebel220e
        GE502014
        )

set(yearTestsModels
        AOSmithHPTU80
        Sanden80
        GE502014
        Rheem2020Prem40
        Rheem2020Prem50
        Rheem2020Build50
        RheemPlugInDedicated50
        RheemPlugInShared40
        RheemPlugInShared50
        AOSmithCAHP120
        AWHSTier3Generic80
        )
set(yearTests
        testCA_3BR_CTZ15
        testCA_3BR_CTZ16
        )

set(maxTempModels
        AOSmithHPTU80
        Rheem2020Build50
        RheemHB50
        Stiebel220e
        GE502014
        TamScalable_SP
        AOSmithCAHP120
        )

set(maxTempTests
        testREGoesTo93C
        testREGoesTo93CCold
        )

set(largeCompressorTests
        testLargeComp45
        testLargeComp60
        testLargeCompHot
        )
set(largeCompressorNames
        QAHV_N136TAU_HPB_SP

        AOSmithCAHP120
        ColmacCxV_5_SP
        ColmacCxA_10_SP
        ColmacCxA_15_SP
        ColmacCxA_20_SP
        ColmacCxA_25_SP
        ColmacCxA_30_SP

        ColmacCxV_5_MP
        ColmacCxA_20_MP
        RheemHPHD60
        RheemHPHD135

        NyleC25A_SP
        NyleC90A_SP
        NyleC185A_SP
        NyleC250A_SP
        NyleC90A_C_SP
        NyleC185A_C_SP
        NyleC250A_C_SP

        NyleC90A_MP
        NyleC250A_MP

        NyleC90A_C_MP
        NyleC250A_C_MP

        TamScalable_SP_Half
        #Scalable_MP
        )

set(yearLargeTestsModels
        QAHV_N136TAU_HPB_SP

        ColmacCxV_5_SP
        ColmacCxA_10_SP
        ColmacCxA_15_SP
        ColmacCxA_20_SP
        ColmacCxA_25_SP
        ColmacCxA_30_SP
        NyleC60A_SP
        NyleC90A_SP
        NyleC185A_SP
        NyleC250A_SP

        ColmacCxV_5_MP
        ColmacCxA_15_MP
        ColmacCxA_20_MP

        NyleC60A_MP
        NyleC185A_MP
        NyleC250A_MP

        RheemHPHD60
        RheemHPHD135
        TamScalable_SP
        Scalable_MP
        )
set(heatExchangeModelNames
        AquaThermAire
        )
set(heatExchangeTests
        villara_24hr67
        )
set(yearLargeTests
        testCA_36Unit_CTZ12
        )

set(stateOfChargeTests
        testSoC70
        testSoCLockout
        testSoCHighEnteringWater
        testSoCSetpointChange
        )
set(stateOfChargeCompressorNames
        Sanden120
        NyleC25A_SP
        )


list(APPEND extendedModelNames ${modelNames} ${heatExchangeModelNames})
list(REMOVE_DUPLICATES extendedModelNames)

#Tests for successful runs
function(add_model_test)
    set(options)
    set(oneValueArgs SPEC_TYPE MODEL_NAME TEST_NAME)
    set(multValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(hpwhArgs run -s ${ARG_SPEC_TYPE} -m ${ARG_MODEL_NAME} -t ${ARG_TEST_NAME} -d ${CMAKE_CURRENT_BINARY_DIR}/output)

    if (${ARG_TEST_NAME} STREQUAL "testLockout")
        if (${ARG_MODEL_NAME} STREQUAL "AOSmithPHPT60")
            set(airTemp "48")
        elseif (${ARG_MODEL_NAME} STREQUAL "AOSmithHPTU80")
            set(airTemp "45")
        elseif (${ARG_MODEL_NAME} STREQUAL "RheemHB50")
            set(airTemp "43")
        elseif (${ARG_MODEL_NAME} STREQUAL "Stiebel220e")
            set(airTemp "35")
        elseif (${ARG_MODEL_NAME} STREQUAL "GE502014")
            set(airTemp "40")
        else ()
            return()
        endif ()
        set(hpwhArgs ${hpwhArgs} -a ${airTemp})
    endif ()

    set(TEST_TITLE "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.${ARG_SPEC_TYPE}")

    add_test(NAME "${TEST_TITLE}" COMMAND $<TARGET_FILE:hpwh> ${hpwhArgs}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
endfunction()

foreach (test ${testNames})
    foreach (model ${modelNames})
        add_model_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
        add_model_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "File")
    endforeach (model)
endforeach (test)

#Test(s) specifically for heat-exchange models
foreach (test ${heatExchangeTests})
    foreach (model ${heatExchangeModelNames})
        add_model_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
        add_model_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "File")
    endforeach (model)
endforeach (test)

#Test models follow control logic with max temperatures.
foreach (test ${maxTempTests})
    foreach (model ${maxTempModels})
        add_model_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

#Add large compressor tests to the list
foreach (test ${largeCompressorTests})
    foreach (model ${largeCompressorNames})
        add_model_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
        #No files for these yet
        #add_model_test( TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE"File")
    endforeach (model)
endforeach (test)

#Add State of Charge tests to the list
foreach (test ${stateOfChargeTests})
    foreach (model ${stateOfChargeCompressorNames})
        add_model_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
        #No files for these yet
        #add_model_test( TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE"File")
    endforeach (model)
endforeach (test)

#Add small compressor yearly tests to the list
foreach (test ${yearTests})
    foreach (model ${yearTestsModels})
        add_model_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

#Add LARGE compressor yearly tests to the list
foreach (test ${yearLargeTests})
    foreach (model ${yearLargeTestsModels})
        add_model_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

# Tests for differences between File and Preset versions
function(add_file_test)
    set(options)
    set(oneValueArgs MODEL_NAME TEST_NAME)
    set(multValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if ((${ARG_TEST_NAME} STREQUAL "testLockout") AND (NOT ${ARG_MODEL_NAME} IN_LIST lockoutTestModels))
        return()
    endif ()

    set(TEST_TITLE "FileTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}")

    add_test(NAME "${TEST_TITLE}" COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_BINARY_DIR}/output/${ARG_TEST_NAME}_File_${ARG_MODEL_NAME}.csv" "${CMAKE_CURRENT_BINARY_DIR}/output/${ARG_TEST_NAME}_Preset_${ARG_MODEL_NAME}.csv"
            )
    set_property(TEST "${TEST_TITLE}" APPEND PROPERTY DEPENDS "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.File")
    set_property(TEST "${TEST_TITLE}" APPEND PROPERTY DEPENDS "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.Preset")
endfunction()

foreach (test ${testNames})
    foreach (model ${modelNames})
        add_file_test(TEST_NAME "${test}" MODEL_NAME "${model}")
    endforeach (model)
endforeach (test)

#Test(s) specifically for heat-exchange models
foreach (test ${heatExchangeTests})
    foreach (model ${heatExchangeModelNames})
        add_file_test(TEST_NAME "${test}" MODEL_NAME "${model}")
    endforeach (model)
endforeach (test)

# Tests for regression differences against reference results
function(add_regression_test)
    set(options)
    set(oneValueArgs SPEC_TYPE MODEL_NAME TEST_NAME)
    set(multValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if ((${ARG_TEST_NAME} STREQUAL "testLockout") AND (NOT ${ARG_MODEL_NAME} IN_LIST lockoutTestModels))
        return()
    endif ()

    set(TEST_TITLE "RegressionTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.${ARG_SPEC_TYPE}")

    add_test(NAME "${TEST_TITLE}" COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_BINARY_DIR}/output/${ARG_TEST_NAME}_${ARG_SPEC_TYPE}_${ARG_MODEL_NAME}.csv" "${CMAKE_CURRENT_SOURCE_DIR}/ref/${ARG_TEST_NAME}_${ARG_SPEC_TYPE}_${ARG_MODEL_NAME}.csv"
            )
    set_property(TEST "${TEST_TITLE}" APPEND PROPERTY DEPENDS "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.File")
    set_property(TEST "${TEST_TITLE}" APPEND PROPERTY DEPENDS "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.Preset")
endfunction()

foreach (test ${testNames})
    foreach (model ${modelNames})
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "File")
    endforeach (model)
endforeach (test)

#Add max temp tests to the regression list
foreach (test ${maxTempTests})
    foreach (model ${maxTempModels})
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

#Add large compressor tests to the regression list
foreach (test ${largeCompressorTests})
    foreach (model ${largeCompressorNames})
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

#Add State of Charge tests to the list
foreach (test ${stateOfChargeTests})
    foreach (model ${stateOfChargeCompressorNames})
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

#Add regression test for yearly file
add_test(NAME "RegressionTest.YearRuns" COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_BINARY_DIR}/output/DHW_YRLY.csv" "${CMAKE_CURRENT_SOURCE_DIR}/ref/DHW_YRLY.csv")

# Add unit tests
add_subdirectory(unit_tests)
