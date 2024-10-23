include(initialize-submodules)

# btwxt library
add_submodule(btwxt)

# googletest library
if (${PROJECT_NAME}_BUILD_TESTING)
    add_submodule(googletest
            TARGET_NAME gtest
            PARENT_SUBMODULE_PATH "${btwxt_SOURCE_DIR}"
            MARK_AS_ADVANCED BUILD_GTEST BUILD_GMOCK INSTALL_GTEST
            CACHE_ON gtest_force_shared_crt BUILD_GTEST BUILD_GMOCK
            CACHE_OFF INSTALL_GTEST
            )
endif ()

# CLI11 library
add_submodule(CLI11)
