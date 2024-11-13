cmake_minimum_required(VERSION 3.14)

project(tph_poissonTests LANGUAGES C CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(tph_poisson REQUIRED)
  enable_testing()
endif()

# Build <float> and <double> versions as static libraries used by the tests.
# The C compiler is used so that linters and other tools recognize that our
# code is in C and can give appropriate warnings.

add_library(tph_poisson_f32 STATIC "src/tph_poisson_f32.c")
add_library(thinks::tph_poisson_f32 ALIAS tph_poisson_f32)
target_link_libraries(tph_poisson_f32 PUBLIC thinks::tph_poisson)
target_compile_features(tph_poisson_f32 PRIVATE c_std_11)

add_library(tph_poisson_f64 STATIC "src/tph_poisson_f64.c")
add_library(thinks::tph_poisson_f64 ALIAS tph_poisson_f64)
target_link_libraries(tph_poisson_f64 PUBLIC thinks::tph_poisson)
target_compile_features(tph_poisson_f64 PRIVATE c_std_11)

# ---- Tests ----

# <f32>
add_executable(tph_poisson_f32_test "src/tph_poisson_test.cpp")
target_link_libraries(tph_poisson_f32_test PRIVATE thinks::tph_poisson_f32)
target_compile_features(tph_poisson_f32_test PRIVATE cxx_std_17)
add_test(NAME tph_poisson_f32_test COMMAND tph_poisson_f32_test)

# <f64>
add_executable(tph_poisson_f64_test "src/tph_poisson_test.cpp")
target_link_libraries(tph_poisson_f64_test PRIVATE thinks::tph_poisson_f64)
target_compile_definitions(tph_poisson_f64_test PRIVATE TPH_POISSON_TEST_USE_F64)
target_compile_features(tph_poisson_f64_test PRIVATE cxx_std_17)
add_test(NAME tph_poisson_f64_test COMMAND tph_poisson_f64_test)

add_executable(tph_poisson_vec_test "src/tph_poisson_vec_test.c")
target_link_libraries(tph_poisson_vec_test PRIVATE thinks::tph_poisson)
target_compile_features(tph_poisson_vec_test PRIVATE c_std_11)
add_test(NAME tph_poisson_vec_test COMMAND tph_poisson_vec_test)
if(NOT MSVC)
  target_link_libraries(tph_poisson_vec_test PRIVATE m)
endif()

add_executable(tph_poisson_alloc_test "src/tph_poisson_alloc_test.c")
target_link_libraries(tph_poisson_alloc_test PRIVATE thinks::tph_poisson)
target_compile_features(tph_poisson_alloc_test PRIVATE c_std_11)
add_test(NAME tph_poisson_alloc_test COMMAND tph_poisson_alloc_test)
if(NOT MSVC)
  target_link_libraries(tph_poisson_alloc_test PRIVATE m)
endif()

add_executable(tph_poisson_libc_test "src/tph_poisson_libc_test.c")
target_link_libraries(tph_poisson_libc_test PRIVATE thinks::tph_poisson)
target_compile_features(tph_poisson_libc_test PRIVATE c_std_11)
add_test(NAME tph_poisson_libc_test COMMAND tph_poisson_libc_test)
if(NOT MSVC)
  target_link_libraries(tph_poisson_libc_test PRIVATE m)
endif()

# ---- End-of-file commands ----

add_folders(Test)
