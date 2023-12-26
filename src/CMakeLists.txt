# Set HPWHsim Version
add_custom_target(${PROJECT_NAME}_version_header
  DEPENDS ${PROJECT_SOURCE_DIR}/src/HPWHversion.in.hh
  COMMAND ${CMAKE_COMMAND}
   ARGS -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
   -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
   -DPROJECT_NAME=${PROJECT_NAME}
   -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
 -P "${PROJECT_SOURCE_DIR}/cmake/git-version.cmake"
)

set_target_properties(${PROJECT_NAME}_version_header PROPERTIES FOLDER Dependencies/HPWHsim)

set(headers
  HPWHversion.in.hh
  HPWH.hh
)

set(source
  HPWHversion.cc
  HPWH.cc
  HPWHHeatSources.cc
  HPWHHeatingLogics.cc
  HPWHpresets.cc
  HPWHSimulator.cc
)
add_library(libHPWHsim ${source} ${headers})

target_include_directories(libHPWHsim PUBLIC ${PROJECT_BINARY_DIR}/src ${PROJECT_SOURCE_DIR}/src)

target_link_libraries( libHPWHsim PUBLIC btwxt)

add_dependencies(libHPWHsim ${PROJECT_NAME}_version_header)

set_target_properties(libHPWHsim PROPERTIES OUTPUT_NAME HPWHsim)
set_target_properties(libHPWHsim PROPERTIES PDB_NAME libHPWHsim)
target_compile_features(libHPWHsim PUBLIC cxx_std_17)

