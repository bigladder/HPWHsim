# Generate Version header

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_branch_exit_status
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT ${git_branch_exit_status} MATCHES "0")
  set(GIT_BRANCH "unknown-branch")
endif()

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-parse --verify --short HEAD
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_sha_exit_status
  OUTPUT_VARIABLE GIT_SHA
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT ${git_sha_exit_status} MATCHES "0")
  set(GIT_SHA "unknown-commit")
endif()

execute_process(
  COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_tag_exit_status
  OUTPUT_VARIABLE GIT_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT ${git_tag_exit_status} MATCHES "0")
  set(GIT_TAG "unknown-tag")
endif()

if (GIT_TAG MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+$")
  string(REGEX REPLACE "^v([0-9]+)\\.[0-9]+\\.[0-9]+$" "\\1" ${PROJECT_NAME}_VRSN_MAJOR "${GIT_TAG}")
  string(REGEX REPLACE "^v[0-9]+\\.([0-9]+)\\.[0-9]+$" "\\1" ${PROJECT_NAME}_VRSN_MINOR "${GIT_TAG}")
  string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+)$" "\\1" ${PROJECT_NAME}_VRSN_PATCH "${GIT_TAG}")
else()
  set(${PROJECT_NAME}_VRSN_MAJOR "?")
  set(${PROJECT_NAME}_VRSN_MINOR "?")
  set(${PROJECT_NAME}_VRSN_PATCH "?")
endif()

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD ^${GIT_TAG}
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE git_build_exit_status
  OUTPUT_VARIABLE GIT_BUILD
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT ${git_build_exit_status} MATCHES "0")
  set(GIT_BUILD "unknown-build-number")
endif()

if(NOT ${GIT_BUILD} MATCHES "^0$")
  set(${PROJECT_NAME}_VRSN_META "+${GIT_BRANCH}.${GIT_SHA}.${GIT_BUILD}")
else()
  set(${PROJECT_NAME}_VRSN_META "")
endif()

message("Building ${PROJECT_NAME} ${${PROJECT_NAME}_VRSN_MAJOR}.${${PROJECT_NAME}_VRSN_MINOR}.${${PROJECT_NAME}_VRSN_PATCH}${${PROJECT_NAME}_VRSN_META}")

configure_file(
  "${PROJECT_SOURCE_DIR}/src/HPWH.in.hh"
  "${PROJECT_BINARY_DIR}/src/HPWH.hh"
)