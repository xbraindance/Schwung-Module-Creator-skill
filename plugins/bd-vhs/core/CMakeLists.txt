add_library(bd_vhs_core STATIC
    src/BdVhsCore.cpp
    src/WowFlutter.cpp
    src/ModelEq.cpp
    src/Saturator.cpp
    src/FailureEngine.cpp
    src/NoiseEngine.cpp
    src/SpreadStage.cpp
    src/AuxPerformance.cpp)

target_include_directories(bd_vhs_core
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_features(bd_vhs_core PUBLIC cxx_std_17)

# Deliberately NOT -ffast-math: it breaks the NaN/Inf detection the tests rely on.
target_compile_options(bd_vhs_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wshadow -Wconversion>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>)

# No target_link_libraries. Zero dependencies is the point of this library:
# it is what makes the DSP buildable and testable on machines with no GUI or
# audio development headers installed.
