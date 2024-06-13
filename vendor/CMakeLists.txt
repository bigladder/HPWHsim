include(initialize-submodules)
#initialize_submodules()

initialize_submodule(btwxt)
if (NOT TARGET btwxt)
  add_subdirectory(btwxt)
endif ()

# googletest library
if (${PROJECT_NAME}_BUILD_TESTING AND NOT TARGET gtest)

    # Prevent GoogleTest from overriding our compiler/linker options
    # when building with Visual Studio
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GTEST ON CACHE BOOL "" FORCE MARK)
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    mark_as_advanced(BUILD_GTEST BUILD_GMOCK INSTALL_GTEST)
	add_subdirectory("${btwxt_SOURCE_DIR}/vendor/googletest")
endif ()

# CLI11 library
if (${PROJECT_NAME}_BUILD_TESTING)
    initialize_submodule(CLI11)
    if (NOT TARGET CLI11)
        add_subdirectory(CLI11)
    endif()
endif()
