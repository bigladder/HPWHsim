# Set HPWHsim Version
add_custom_target(${PROJECT_NAME}_version_header
  DEPENDS ${PROJECT_SOURCE_DIR}/src/HPWH.in.hh
  COMMAND ${CMAKE_COMMAND}
   ARGS -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
   -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
   -DPROJECT_NAME=${PROJECT_NAME}
   -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
 -P "${PROJECT_SOURCE_DIR}/cmake/git-version.cmake"
)

set_target_properties(${PROJECT_NAME}_version_header PROPERTIES FOLDER Dependencies/HPWHsim)
include_directories("${PROJECT_BINARY_DIR}/src")

set(headers
  HPWH.in.hh
)

set(source
  HPWH.cc
  HPWHHeatingLogics.cc
  HPWHpresets.cc
)
add_library(libHPWHsim ${source} ${headers})

add_dependencies(libHPWHsim ${PROJECT_NAME}_version_header)

set_target_properties(libHPWHsim PROPERTIES OUTPUT_NAME HPWHsim)
set_target_properties(libHPWHsim PROPERTIES PDB_NAME libHPWHsim)
target_link_libraries(libHPWHsim PRIVATE hpwhsim_interface_library)
target_compile_features(libHPWHsim PRIVATE cxx_std_17)

target_include_directories(libHPWHsim PUBLIC ${PROJECT_SOURCE_DIR}/vendor/btwxt/src)