# SPDX-FileCopyrightText: © 2026 Big Ladder Software <info@bigladdersoftware.com>
# SPDX-License-Identifier: BSD-3-Clause
include(initialize-submodules)

if (NOT TARGET nlohmann_json)
    add_subdirectory(json)
endif()



if (NOT TARGET courier)
    add_subdirectory(courier)
    mark_as_advanced(courier_BUILD_TESTING courier_COVERAGE courier_STATIC_LIB)
endif ()



