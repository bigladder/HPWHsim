# Build test tool
include_directories("${PROJECT_BINARY_DIR}/src")

add_executable(testTool main.cc )
add_executable(testTankSizeFixed testTankSizeFixed.cc)

target_link_libraries(testTool libHPWHsim)
target_link_libraries(testTankSizeFixed libHPWHsim)

# Add output directory for test results
add_custom_target(results_directory ALL COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/output")

# Clear output file for yearly tests
#add_custom_target(do_always ALL COMMAND ${CMAKE_COMMAND} file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/output/DHW_YRLY.csv") )
add_test(NAME "Ready.Output.Files" COMMAND ${CMAKE_COMMAND} -E remove "${CMAKE_CURRENT_BINARY_DIR}/output/DHW_YRLY.csv")

# Run tests
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
  AOSmithPHPT60
  AOSmithHPTU80
  Sanden80
  RheemHB50
  Stiebel220e
  GE502014
  Rheem2020Prem40
  Rheem2020Prem50
  Rheem2020Build50
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
 AOSmithCAHP120
)                                  
set(yearTests                                        
 testCA_3BR_CTZ15
 testCA_3BR_CTZ16
)

set(largeCompressorTests
 testLargeComp45
 testLargeComp60
 testLargeCompHot
)
set(largeCompressorNames
 AOSmithCAHP120
 ColmacCxV_5_SP
 ColmacCxA_10_SP
 ColmacCxA_15_SP
 ColmacCxA_20_SP
 ColmacCxA_25_SP
 ColmacCxA_30_SP
 NyleC25A_SP
 NyleC90A_SP 
 NyleC185A_SP
 NyleC250A_SP
 NyleC90A_C_SP 
 NyleC185A_C_SP
 NyleC250A_C_SP
)

set(yearLargeTestsModels   
 ColmacCxV_5_SP
 ColmacCxA_10_SP
 ColmacCxA_15_SP
 ColmacCxA_20_SP
 ColmacCxA_25_SP
 ColmacCxA_30_SP
 NyleC90A_SP
 NyleC185A_SP
 NyleC250A_SP
)                                  
set(yearLargeTests                                        
 testCA_36Unit_CTZ12
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
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
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