add_executable(bd_vhs_tests TestMain.cpp)

target_link_libraries(bd_vhs_tests PRIVATE bd_vhs_core)
target_include_directories(bd_vhs_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# -Wmismatched-new-delete fires on the allocation watchdog's global operator
# new/delete pair: GCC inlines them and loses track of the fact that they are
# matched. The pairing is correct -- malloc in new, free in delete.
target_compile_options(bd_vhs_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-mismatched-new-delete>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>)

add_test(NAME bdvhs_core COMMAND bd_vhs_tests)
