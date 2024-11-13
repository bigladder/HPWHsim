# Build unity
set(source_files
        unity.hpp
        units.hpp
        unity.cpp
        )

add_library(unity ${source_files})

target_compile_features(unity PRIVATE cxx_std_17)
