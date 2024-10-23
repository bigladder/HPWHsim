include(initialize-submodules)

# btwxt library
add_submodule(btwxt)

if (${PROJECT_NAME}_BUILD_TESTING)

    # googletest library
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GTEST ON CACHE BOOL "" FORCE MARK)
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    mark_as_advanced(BUILD_GTEST BUILD_GMOCK INSTALL_GTEST)
	  add_submodule(googletest gtest "${btwxt_SOURCE_DIR}")
endif()

# CLI11 library
add_submodule(CLI11)

# JSON library
add_submodule(json)
