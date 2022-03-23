

include(initialize-submodules)
initialize_submodules()

if (NOT TARGET btwxt)
  add_subdirectory(btwxt)
endif ()