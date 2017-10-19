# Build test tool
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

include_directories("${CMAKE_SOURCE_DIR}/src")

add_executable(testTool.x main.cc)

target_link_libraries(testTool.x libHPWHsim)
