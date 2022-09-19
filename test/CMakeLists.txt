# Build test tool
include_directories("${PROJECT_BINARY_DIR}/src")

add_executable(testTool main.cc )
add_executable(testTankSizeFixed testTankSizeFixed.cc)
add_executable(testScaleHPWH testScaleHPWH.cc)
add_executable(testMaxSetpoint testMaxSetpoint.cc)
add_executable(testSizingFractions testSizingFractions.cc)
add_executable(testResistanceFcts testResistanceFcts.cc)
add_executable(testCompressorFcts testCompressorFcts.cc)
add_executable(testPerformanceMaps testPerformanceMaps.cc)

set(libs
 libHPWHsim 
 btwxt
)
target_link_libraries(testTool ${libs})
target_link_libraries(testTankSizeFixed ${libs})
target_link_libraries(testScaleHPWH ${libs})
target_link_libraries(testMaxSetpoint ${libs})
target_link_libraries(testSizingFractions ${libs})
target_link_libraries(testResistanceFcts ${libs})
target_link_libraries(testCompressorFcts ${libs})
target_link_libraries(testPerformanceMaps ${libs})


# Add output directory for test results
add_custom_target(results_directory ALL COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/output")

# Clear output file for yearly tests
#add_custom_target(do_always ALL COMMAND ${CMAKE_COMMAND} file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/output/DHW_YRLY.csv") )
add_test(NAME "Ready.Output.Files" COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_CURRENT_BINARY_DIR}/output/DHW_YRLY.csv")

# Run Unit tests
add_test(NAME "testScaleHPWH" COMMAND  $<TARGET_FILE:testScaleHPWH> ${testArgs} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME "testMaxSetpoint" COMMAND  $<TARGET_FILE:testMaxSetpoint> ${testArgs} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME "testSizingFractions" COMMAND  $<TARGET_FILE:testSizingFractions> ${testArgs} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME "testResistanceFcts" COMMAND  $<TARGET_FILE:testResistanceFcts> ${testArgs} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME "testCompressorFcts" COMMAND  $<TARGET_FILE:testCompressorFcts> ${testArgs} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME "testPerformanceMaps" COMMAND  $<TARGET_FILE:testPerformanceMaps> ${testArgs} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

#add_test(NAME "testREGoesTo99C.AOSmithCAHP120" COMMAND $<TARGET_FILE:testTool> "Preset" "AOSmithCAHP120" "testREGoesTo99C"
#"${CMAKE_CURRENT_BINARY_DIR}/output" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#add_test(NAME "testREGoesTo99CCold.AOSmithCAHP120" COMMAND $<TARGET_FILE:testTool> "Preset" "AOSmithCAHP120" "testREGoesTo99CCold"
#"${CMAKE_CURRENT_BINARY_DIR}/output" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Run model tests and unit tests on models
set(testNames
 test24hr50
 test24hr95
)
set(modelNames
#  StorageTank  
#  AOSmithPHPT60
#  AOSmithHPTU80
#  Sanden80
#  RheemHB50
#  Stiebel220e
#  GE502014
#  Rheem2020Prem40
#  Rheem2020Prem50
#  Rheem2020Build50
  RheemPlugInDedicated50
)
set(lockoutTestModels
#  AOSmithPHPT60
#  AOSmithHPTU80
#  RheemHB50
#  Stiebel220e
#  GE502014
)

set(yearTestsModels          
 AOSmithHPTU80
 Sanden80       
 GE502014
 Rheem2020Prem40 
 Rheem2020Prem50
 Rheem2020Build50
 AOSmithCAHP120
 AWHSTier3Generic80
)                                  
set(yearTests                                        
# testCA_3BR_CTZ15
# testCA_3BR_CTZ16
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

set( maxTempTests
# testREGoesTo93C
# testREGoesTo93CCold
)

set(largeCompressorTests
# testLargeComp45
# testLargeComp60
# testLargeCompHot
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

set(yearLargeTests                                        
# testCA_36Unit_CTZ12
)

# Unit tests
function( add_TankSizeFixed_test )
  set(options)
  set(oneValueArgs MODEL_NAME)
  set(multValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  set(testArgs "${ARG_MODEL_NAME}")

  set(TEST_TITLE "TankSizeFixed.${ARG_MODEL_NAME}")

  add_test(NAME "${TEST_TITLE}" COMMAND  $<TARGET_FILE:testTankSizeFixed> ${testArgs}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

foreach(model ${modelNames})
	add_TankSizeFixed_test( MODEL_NAME "${model}")
endforeach(model)
foreach(model ${largeCompressorNames})
	add_TankSizeFixed_test( MODEL_NAME "${model}")
endforeach(model)

#Tests for successful runs
function( add_model_test )
  set(options)
  set(oneValueArgs INP_SOURCE MODEL_NAME TEST_NAME)
  set(multValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  set(testToolArgs "${ARG_INP_SOURCE}" "${ARG_MODEL_NAME}" "${ARG_TEST_NAME}" "${CMAKE_CURRENT_BINARY_DIR}/output")

  if(${ARG_TEST_NAME} STREQUAL "testLockout")
    if( ${ARG_MODEL_NAME} STREQUAL "AOSmithPHPT60" )
      set(airTemp "48")
    elseif( ${ARG_MODEL_NAME} STREQUAL "AOSmithHPTU80" )
      set(airTemp "45")
    elseif( ${ARG_MODEL_NAME} STREQUAL "RheemHB50" )
      set(airTemp "43")
    elseif( ${ARG_MODEL_NAME} STREQUAL "Stiebel220e" )
      set(airTemp "35")
    elseif( ${ARG_MODEL_NAME} STREQUAL "GE502014" )
      set(airTemp "40")
    else()
      return()
    endif()
    set(testToolArgs ${testToolArgs} ${airTemp})
  endif()

  set(TEST_TITLE "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.${ARG_INP_SOURCE}")

  add_test(NAME "${TEST_TITLE}" COMMAND $<TARGET_FILE:testTool> ${testToolArgs}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endfunction()

foreach(test ${testNames})
  foreach(model ${modelNames})
    add_model_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "Preset")
    add_model_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "File")
  endforeach(model)
endforeach(test)
#Test models follow control logic with max temperatures. 
foreach(test ${maxTempTests})
  foreach(model ${maxTempModels})
    add_model_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "Preset")
  endforeach(model)
endforeach(test)
#Add large compressor tests to the list
foreach(test ${largeCompressorTests})
  foreach(model ${largeCompressorNames})
    add_model_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "Preset")
	#No files for these yet
    #add_model_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "File")
  endforeach(model)
endforeach(test)

#Add small compressor yearly tests to the list
foreach(test ${yearTests})
  foreach(model ${yearTestsModels})
    add_model_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "Preset")
  endforeach(model)
endforeach(test)

#Add LARGE compressor yearly tests to the list
foreach(test ${yearLargeTests})
  foreach(model ${yearLargeTestsModels})
    add_model_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "Preset")
  endforeach(model)
endforeach(test)


# Tests for differences between File and Preset versions
function( add_file_test )
  set(options)
  set(oneValueArgs MODEL_NAME TEST_NAME)
  set(multValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if ((${ARG_TEST_NAME} STREQUAL "testLockout") AND (NOT ${ARG_MODEL_NAME}  IN_LIST lockoutTestModels))
    return()
  endif()

  set(TEST_TITLE "FileTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}")

  add_test(NAME "${TEST_TITLE}" COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_BINARY_DIR}/output/${ARG_TEST_NAME}_File_${ARG_MODEL_NAME}.csv" "${CMAKE_CURRENT_BINARY_DIR}/output/${ARG_TEST_NAME}_Preset_${ARG_MODEL_NAME}.csv"
  )
  set_property(TEST "${TEST_TITLE}" APPEND PROPERTY DEPENDS "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.File")
  set_property(TEST "${TEST_TITLE}" APPEND PROPERTY DEPENDS "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.Preset")
endfunction()

foreach(test ${testNames})
  foreach(model ${modelNames})
    add_file_test( TEST_NAME "${test}" MODEL_NAME "${model}")
  endforeach(model)
endforeach(test)

# Tests for regression differences against reference results
function( add_regression_test )
  set(options)
  set(oneValueArgs INP_SOURCE MODEL_NAME TEST_NAME)
  set(multValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if ((${ARG_TEST_NAME} STREQUAL "testLockout") AND (NOT ${ARG_MODEL_NAME}  IN_LIST lockoutTestModels))
    return()
  endif()

  set(TEST_TITLE "RegressionTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.${ARG_INP_SOURCE}")

  add_test(NAME "${TEST_TITLE}" COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_BINARY_DIR}/output/${ARG_TEST_NAME}_${ARG_INP_SOURCE}_${ARG_MODEL_NAME}.csv" "${CMAKE_CURRENT_SOURCE_DIR}/ref/${ARG_TEST_NAME}_${ARG_INP_SOURCE}_${ARG_MODEL_NAME}.csv"
  )
  set_property(TEST "${TEST_TITLE}" APPEND PROPERTY DEPENDS "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.File")
  set_property(TEST "${TEST_TITLE}" APPEND PROPERTY DEPENDS "ModelTest.${ARG_TEST_NAME}.${ARG_MODEL_NAME}.Preset")
endfunction()

foreach(test ${testNames})
  foreach(model ${modelNames})
    add_regression_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "Preset")
    add_regression_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "File")
  endforeach(model)
endforeach(test)

#Add max temp tests to the regression list
foreach(test ${maxTempTests})
  foreach(model ${maxTempModels})
    add_regression_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "Preset")
	#No files for these yet
    #add_regression_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "File")
  endforeach(model)
endforeach(test)


#Add large compressor tests to the regression list
foreach(test ${largeCompressorTests})
  foreach(model ${largeCompressorNames})
    add_regression_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "Preset")
	#No files for these yet
    #add_regression_test( TEST_NAME "${test}" MODEL_NAME "${model}" INP_SOURCE "File")
  endforeach(model)
endforeach(test)

#Add regression test for yearly file 
add_test(NAME "RegressionTest.YearRuns" COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_BINARY_DIR}/output/DHW_YRLY.csv" "${CMAKE_CURRENT_SOURCE_DIR}/ref/DHW_YRLY.csv")