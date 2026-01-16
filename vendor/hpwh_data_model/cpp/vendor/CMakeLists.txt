#include(initialize-submodules)
#initialize_submodules()




if (NOT TARGET nlohmann_json)
    add_subdirectory(json)
endif()





if (NOT TARGET courier)
    add_subdirectory(courier)
    mark_as_advanced(courier_BUILD_TESTING courier_COVERAGE courier_STATIC_LIB)
endif ()





if (NOT TARGET fmt)
    add_subdirectory(fmt)
    set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
    mark_as_advanced(FMT_CMAKE_DIR FMT_CUDA_TEST FMT_DEBUG_POSTFIX FMT_DOC FMT_FUZZ FMT_INC_DIR FMT_INSTALL FMT_INSTALL
            FMT_LIB_DIR FMT_MODULE FMT_OS FMT_PEDANTIC FMT_PKGCONFIG_DIR FMT_SYSTEM_HEADERS FMT_TEST FMT_WERROR)
endif ()



