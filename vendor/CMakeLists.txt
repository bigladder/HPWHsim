include(initialize-submodules)

# btwxt library
add_submodule(btwxt)

# message(STATUS "Adding subdirectory \"Unity\"")
add_submodule(Unity)

if (${PROJECT_NAME}_BUILD_TESTING)

    # googletest library
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GTEST ON CACHE BOOL "" FORCE MARK)
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    mark_as_advanced(BUILD_GTEST BUILD_GMOCK INSTALL_GTEST)
	add_submodule(googletest gtest "${btwxt_SOURCE_DIR}")

    # CLI11 library
    add_submodule(CLI11)
endif ()
