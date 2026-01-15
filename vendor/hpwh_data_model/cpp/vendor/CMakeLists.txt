# SPDX-FileCopyrightText: © 2026 Big Ladder Software <info@bigladdersoftware.com>
# SPDX-License-Identifier: BSD-3-Clause
include(initialize-submodules)
add_submodule(fmt
        MARK_AS_ADVANCED
        FMT_CMAKE_DIR FMT_CUDA_TEST FMT_DEBUG_POSTFIX FMT_DOC FMT_FUZZ FMT_INC_DIR FMT_INSTALL FMT_INSTALL FMT_LIB_DIR FMT_MODULE FMT_OS FMT_PEDANTIC FMT_PKGCONFIG_DIR FMT_SYSTEM_HEADERS FMT_TEST FMT_WERROR
        CACHE_OFF FMT_INSTALL
)

add_submodule(googletest
        TARGET_NAME gtest
        MARK_AS_ADVANCED BUILD_GTEST BUILD_GMOCK INSTALL_GTEST
        CACHE_ON gtest_force_shared_crt BUILD_GTEST BUILD_GMOCK
        CACHE_OFF INSTALL_GTEST

if (NOT TARGET courier)
    add_subdirectory(courier)
endif()

if (NOT TARGET json)
    add_subdirectory(json)
endif()
