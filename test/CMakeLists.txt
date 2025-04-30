# Add output directory for test results
add_custom_target(results_directory ALL COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/output")

add_test(NAME "Ready.Output.Files" COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_CURRENT_BINARY_DIR}/output/DHW_YRLY.csv")

# Run model tests and unit tests on models
set(modelNames
        #  StorageTank
        AOSmithHPTS40
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

set(general_TestDir "general_tests")
set(general_Tests
        test30
        test50
        test70
        test95
        testSandenCombi
        )

# Demand-response tests
set(DR_TestDir "DR_tests")
set(DR_Tests
        testDr_LO
        testDr_TOO
        testDr_TOT
        testDr_TOO2
        testDR_TOTLOR
        )

# lockout test
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

set(maxTempTests
        testREGoesTo93C
        testREGoesTo93CCold
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

set(largeCompressor_TestDir "large_compressor_tests")
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

#
set(SoC_Models
        Sanden120
        NyleC25A_SP
        )
set(SoC_TestDir "SoC_tests")
set(SoC_Tests
        testSoC70
        testSoCLockout
        testSoCHighEnteringWater
        testSoCSetpointChange
        )

#
set(Villara_Models "AquaThermAire")
set(Villara_TestDir "Villara/AquaThermAire")
set(Villara_Tests
        villara_24hr67
        )

#
set(BradfordWhiteModels
        AeroThermRE2H50
        AeroThermRE2H65
        AeroThermRE2H80
        )
set(BradfordWhiteAeroThermRE2H50_Tests
        RE2H50_UEF50
        RE2H50_UEF67
        )
set(BradfordWhiteAeroThermRE2H65_Tests
        RE2H65_UEF50
        RE2H65_UEF67
        )
set(BradfordWhiteAeroThermRE2H80_Tests
        RE2H80_UEF50
        RE2H80_UEF67
        )

#
set(LG_Models LG_APHWC50 LG_APHWC80)
set(LG_TestDir "LG/APHWC50")
set(LG_Tests
        DOE2014_LGC50_24hr50
        DOE2014_LGC50_24hr67
        DOE2014_LGC50_24hr95
        DP_LGC50_HVT
        )

cmake_dependent_option(${PROJECT_NAME}_PYTHON_PLOTS "Generate plots using python comparing measured and simulated data" ON "${PROJECT_NAME}_IS_TOP_LEVEL" OFF)
if (${PROJECT_NAME}_PYTHON_PLOTS AND NOT ${PROJECT_NAME}_PYTHON_PLOTS_SETUP)
    set(pythonScriptDir "${PROJECT_SOURCE_DIR}/scripts/python/plotting")
    include(poetry-install)
    poetry_install(${pythonScriptDir})
    set(${PROJECT_NAME}_PYTHON_PLOTS_SETUP TRUE CACHE BOOL "Python plotting environment is already setup" FORCE)
endif ()

#Tests for successful runs
function(add_model_test)
    set(options)
    set(oneValueArgs SPEC_TYPE MODEL_NAME TEST_NAME AIR_T_F)
    set(multValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(hpwhArgs run -s ${ARG_SPEC_TYPE} -m ${ARG_MODEL_NAME} -t ${ARG_TEST_NAME} -d ${CMAKE_CURRENT_BINARY_DIR}/output)

    if (DEFINED ARG_AIR_T_F)
        set(hpwhArgs ${hpwhArgs} -a ${ARG_AIR_T_F})
    endif()

    set(TEST_TITLE "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.${ARG_SPEC_TYPE}")

    add_test(NAME "${TEST_TITLE}" COMMAND $<TARGET_FILE:hpwh> ${hpwhArgs}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )

endfunction()

# Tests for regression differences against reference results
function(add_regression_test)
    set(options)
    set(oneValueArgs TEST_NAME SPEC_TYPE MODEL_NAME COMPARE_SPEC_TYPE)
    set(multValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (DEFINED ARG_COMPARE_SPEC_TYPE)
        set(compareSpecType ${ARG_COMPARE_SPEC_TYPE})
    else()
        set(compareSpecType ${ARG_SPEC_TYPE})
    endif()

    set(TEST_TITLE "RegressionTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.${ARG_SPEC_TYPE}.${compareSpecType}")

    # get subfolder name
    string(REPLACE "/" ";" test_name_list "${ARG_TEST_NAME}")
    list(LENGTH test_name_list ntest_name_terms)
    if (ntest_name_terms GREATER 0)
        list(GET test_name_list -1 test_prefix)
    else()
        set(test_prefix "${ARG_TEST_NAME}")
    endif()

    add_test(NAME "${TEST_TITLE}" COMMAND ${CMAKE_COMMAND} -E compare_files
            "${CMAKE_CURRENT_BINARY_DIR}/output/${test_prefix}_${ARG_SPEC_TYPE}_${ARG_MODEL_NAME}.csv"
            "${CMAKE_CURRENT_SOURCE_DIR}/ref/${test_prefix}_${compareSpecType}_${ARG_MODEL_NAME}.csv"
            )
    #set_property(TEST "${TEST_TITLE}" APPEND PROPERTY DEPENDS "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.Preset")
endfunction()

#Plot test
function(add_plot_test)
    set(options)
    set(oneValueArgs SPEC_TYPE MODEL_NAME TEST_NAME)
    set(multValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(measurementsPath "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_TEST_NAME}/measured.csv")
    if (EXISTS ${measurementsPath})

        # get subfolder name
        string(REPLACE "/" ";" test_name_list "${ARG_TEST_NAME}")
        list(LENGTH test_name_list ntest_name_terms)
        if (ntest_name_terms GREATER 0)
            list(GET test_name_list -1 test_prefix)
        else()
            set(test_prefix "${ARG_TEST_NAME}")
        endif()

        set(simulationOutputName "${test_prefix}_${ARG_SPEC_TYPE}_${ARG_MODEL_NAME}")
        set(simulationOutputPath "${CMAKE_CURRENT_BINARY_DIR}/output/${simulationOutputName}.csv")
        set(simulationPlotPath "${CMAKE_CURRENT_BINARY_DIR}/output/${simulationOutputName}.html")

        set(plotTestName "Plot.${ARG_TEST_NAME}.${ARG_MODEL_NAME}")
        add_test(NAME "${plotTestName}" COMMAND poetry -C "${PROJECT_SOURCE_DIR}/scripts/python/plotting" run python
                "${PROJECT_SOURCE_DIR}/scripts/python/plotting/test_plot.py" ${measurementsPath} ${simulationOutputPath} ${simulationPlotPath} )
        set_property(TEST "${plotTestName}" APPEND PROPERTY DEPENDS "${TEST_TITLE}")
    endif ()

endfunction()

## Model tests
# general tests
foreach (test ${general_Tests})
    foreach (model ${modelNames})
				set(fullTestName "${general_TestDir}/${test}")
        add_model_test(TEST_NAME "${fullTestName}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

# lockout tests
foreach (model ${lockoutTestModels})
  if (${model} STREQUAL "AOSmithPHPT60")
      set(airT_F "48")
  elseif (${model} STREQUAL "AOSmithHPTU80")
      set(airT_F "45")
  elseif (${model} STREQUAL "RheemHB50")
      set(airT_F "43")
  elseif (${model} STREQUAL "Stiebel220e")
      set(airT_F "35")
  elseif (${model} STREQUAL "GE502014")
      set(airT_F "40")
	else ()
		continue()
  endif ()

	add_model_test(TEST_NAME testLockout MODEL_NAME "${model}" SPEC_TYPE "Preset" AIR_T_F "${airT_F}")
endforeach (model)

# DR tests
foreach (test ${DR_Tests})
    foreach (model ${modelNames})
				set(fullTestName "${DR_TestDir}/${test}")
				add_model_test(TEST_NAME "${fullTestName}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

# Villara AquaThermAire tests
foreach (model ${Villara_Models})
    foreach (test ${Villara_Tests})
				set(fullTestName "${Villara_TestDir}/${test}")
        add_model_test(TEST_NAME "${fullTestName}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
        add_model_test(TEST_NAME "${fullTestName}" MODEL_NAME "${model}" SPEC_TYPE "JSON")
    endforeach (test)
endforeach (model)

## Plot test
add_plot_test(SPEC_TYPE "Preset" MODEL_NAME "AquaThermAire" TEST_NAME "Villara/AquaThermAire/villara_24hr67" )

# Bradford White AeroTherm tests
foreach (model ${BradfordWhiteModels})
    foreach (test "${BradfordWhite${model}_Tests}")
		set(fullTestName "BradfordWhite/${model}/${test}")
        add_model_test(TEST_NAME "${fullTestName}" MODEL_NAME "BradfordWhite${model}" SPEC_TYPE "Preset")
        add_model_test(TEST_NAME "${fullTestName}" MODEL_NAME "BradfordWhite${model}" SPEC_TYPE "JSON")
    endforeach (test)
endforeach (model)

# LG tests
foreach(model ${LG_Models})
    foreach (test ${LG_Tests})
        set(fullTestName "${LG_TestDir}/${test}")
        add_model_test(TEST_NAME "${fullTestName}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
        add_model_test(TEST_NAME "${fullTestName}" MODEL_NAME "${model}" SPEC_TYPE "JSON")
    endforeach (test)
endforeach (model)

# Test models follow control logic with max temperatures.
foreach (test ${maxTempTests})
    foreach (model ${maxTempModels})
        add_model_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

# State-of-Charge tests 
foreach (test ${SoC_Tests})
    foreach (model ${SoC_Models})
			set(fullTestName "${SoC_TestDir}/${test}")
        add_model_test(TEST_NAME "${fullTestName}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

# Large-compressor tests
foreach (test ${largeCompressorTests})
    foreach (model ${largeCompressorNames})
        set(fullTestName "${largeCompressor_TestDir}/${test}")
        add_model_test(TEST_NAME "${fullTestName}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
        add_model_test(TEST_NAME "${fullTestName}" MODEL_NAME "${model}" SPEC_TYPE "JSON")
    endforeach (model)
endforeach (test)

## Regression tests
# general regression tests
foreach (test ${general_Tests})
    foreach (model ${modelNames})
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

# lockout test regressions
foreach (model ${lockoutTestModels})
  add_regression_test(TEST_NAME testLockout MODEL_NAME "${model}" SPEC_TYPE "Preset")
endforeach (model)

# DR regression tests
foreach (test ${DR_Tests})
    foreach (model ${modelNames})
			set(fullTestName "${DR_TestDir}/${test}")
       add_regression_test(TEST_NAME "${fullTestName}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

# Max temp regression tests
foreach (test ${maxTempTests})
    foreach (model ${maxTempModels})
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

# State-of-charge regression tests
foreach (test ${SoC_Tests})
    foreach (model ${SoC_Models})
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

# Villara Aerotherm regression tests
foreach (model ${Villara_Models})
    foreach (test ${Villara_Tests})
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "JSON" COMPARE_SPEC_TYPE "Preset")
    endforeach (test)
endforeach (model)

# BradfordWhite regression tests
foreach (model ${BradfordWhiteModels})
    foreach (test "${BradfordWhite${model}_Tests}")
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "BradfordWhite${model}" SPEC_TYPE "Preset")
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "BradfordWhite${model}" SPEC_TYPE "JSON" COMPARE_SPEC_TYPE "Preset")
    endforeach (test)
endforeach (model)

# LG regression tests
foreach (model ${LG_Models})
    foreach (test ${LG_Tests})
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "JSON" COMPARE_SPEC_TYPE "Preset")
    endforeach (test)
endforeach (model)

# Large-compressor regression tests
foreach (test ${largeCompressorTests})
    foreach (model ${largeCompressorNames})
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "Preset")
        add_regression_test(TEST_NAME "${test}" MODEL_NAME "${model}" SPEC_TYPE "JSON" COMPARE_SPEC_TYPE "Preset")
    endforeach (model)
endforeach (test)

## Yearly tests
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

#Add regression test for yearly file
add_test(NAME "RegressionTest.YearRuns" COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_BINARY_DIR}/output/DHW_YRLY.csv" "${CMAKE_CURRENT_SOURCE_DIR}/ref/DHW_YRLY.csv")

# Add unit tests
add_subdirectory(unit_tests)
