# Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
# See the LICENSE.txt file for additional terms and conditions.

set(source_files
	HPWHapp.cpp
	runCommand.cpp
	makeCommand.cpp
	measureCommand.cpp
)

add_executable(hpwh ${source_files})


target_include_directories(hpwh
	PUBLIC "${PROJECT_SOURCE_DIR}/src"
	PRIVATE "${PROJECT_SOURCE_DIR}/vendor/CLI11")

target_link_libraries(hpwh HPWHsim CLI11)