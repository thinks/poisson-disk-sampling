cmake_minimum_required(VERSION 3.14)

project(tph_poissonTests LANGUAGES C CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(tph_poisson REQUIRED)
  enable_testing()
endif()

add_library(tph_poisson_f STATIC "src/tph_poisson_f.c")
add_library(thinks::tph_poisson_f ALIAS tph_poisson_f)
target_link_libraries(tph_poisson_f PUBLIC thinks::tph_poisson)
target_compile_features(tph_poisson_f PRIVATE c_std_11)

add_library(tph_poisson_d STATIC "src/tph_poisson_d.c")
add_library(thinks::tph_poisson_d ALIAS tph_poisson_d)
target_link_libraries(tph_poisson_d PUBLIC thinks::tph_poisson)
target_compile_features(tph_poisson_d PRIVATE c_std_11)

#include(../cmake/fetch-rpmalloc.cmake)
#fetch_rpmalloc(VERSION "6800c43fb2f22626bb043369698ee862e60710c2")

# ---- Tests ----

# add_executable(tph_poisson_test src/tph_poisson_test.cpp)
# # set_property(
# #     SOURCE src/tph_poisson_test.cpp PROPERTY
# #     COMPILE_DEFINITIONS INIT_TEST_IMPLEMENTATION
# # )
# target_link_libraries(tph_poisson_test PRIVATE thinks::tph_poisson)
# target_compile_features(tph_poisson_test PRIVATE cxx_std_17)

# add_test(NAME tph_poisson_test COMMAND tph_poisson_test)

# <float>
add_executable(tph_poisson_f_test "src/tph_poisson_test.cpp")
target_link_libraries(tph_poisson_f_test PRIVATE thinks::tph_poisson_f)# rpmalloc)
target_compile_features(tph_poisson_f_test PRIVATE c_std_11)
target_compile_features(tph_poisson_f_test PRIVATE cxx_std_17)
add_test(NAME tph_poisson_f_test COMMAND tph_poisson_f_test)

# <double>
add_executable(tph_poisson_d_test "src/tph_poisson_test.cpp")
target_link_libraries(tph_poisson_d_test PRIVATE thinks::tph_poisson_d)# rpmalloc)
target_compile_definitions(tph_poisson_d_test PRIVATE TPH_POISSON_TEST_USE_DOUBLE)
target_compile_features(tph_poisson_d_test PRIVATE c_std_11)
target_compile_features(tph_poisson_d_test PRIVATE cxx_std_17)
add_test(NAME tph_poisson_d_test COMMAND tph_poisson_d_test)

# ---- End-of-file commands ----

add_folders(Test)


# # Default to c++20.
# if(DEFINED CMAKE_CXX_STANDARD)
#   set(_cxx_standard ${CMAKE_CXX_STANDARD})
# else()
#   set(_cxx_standard 20)
# endif()

# # Default to c11.
# if(DEFINED CMAKE_C_STANDARD)
#   set(_c_standard ${CMAKE_C_STANDARD})
# else()
#   set(_c_standard 11)
# endif()

# if(MSVC)
#   set(_c_warnings
#     /W4 # Baseline reasonable warnings
#     /w14242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
#     /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
#     /w14263 # 'function': member function does not override any base class virtual member function
#     /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not
#             # be destructed correctly
#     /w14287 # 'operator': unsigned/negative constant mismatch
#     /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside
#             # the for-loop scope
#     /w14296 # 'operator': expression is always 'boolean_value'
#     /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
#     /w14545 # expression before comma evaluates to a function which is missing an argument list
#     /w14546 # function call before comma missing argument list
#     /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
#     /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
#     /w14555 # expression has no effect; expected expression with side- effect
#     /w14619 # pragma warning: there is no warning number 'number'
#     /w14640 # Enable warning on thread un-safe static member initialization
#     /w14826 # Conversion from 'type1' to 'type2' is sign-extended. This may cause unexpected runtime behavior.
#     /w14905 # wide string literal cast to 'LPSTR'
#     /w14906 # string literal cast to 'LPWSTR'
#     /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
#     /permissive- # standards conformance mode for MSVC compiler.
#   )
#   if(TPH_POISSON_WARNINGS_AS_ERRORS)
#     message(TRACE "Warnings are treated as errors")
#     list(APPEND _c_warnings /WX)
#   endif()
# elseif(CMAKE_C_COMPILER_ID MATCHES ".*Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
#   set(_c_warnings
#     -Wall
#     -Wextra # reasonable and standard
#     -Wshadow # warn the user if a variable declaration shadows one from a parent context
#     # catch hard to track down memory errors
#     -Wcast-align # warn for potential performance problem casts
#     -Wunused # warn on anything being unused
#     -Wpedantic # warn if non-standard C++ is used
#     -Wconversion # warn on type conversions that may lose data
#     -Wsign-conversion # warn on sign conversions
#     -Wnull-dereference # warn if a null dereference is detected
#     -Wdouble-promotion # warn if float is implicit promoted to double
#     -Wformat=2 # warn on security issues around functions that format output (ie printf)
#     -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
#   )
#   if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
#     set(_c_warnings
#       ${_c_warnings}
#       -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
#       -Wduplicated-cond # warn if if / else chain has duplicated conditions
#       -Wduplicated-branches # warn if if / else branches have duplicated code
#       -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
#     )
#   endif()
#   if(TPH_POISSON_WARNINGS_AS_ERRORS)
#     message(TRACE "Warnings are treated as errors")
#     list(APPEND _c_warnings -Werror)
#   endif()
# else()
#   message(AUTHOR_WARNING "No compiler warnings set for C compiler: '${CMAKE_C_COMPILER_ID}'")
# endif()

# add_library(tph_poisson_asan INTERFACE)
# add_library(thinks::tph_poisson_asan ALIAS tph_poisson_asan)
# if(TPH_POISSON_USE_ASAN)
#   if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
#     target_compile_options(tph_poisson_asan INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer -fsanitize-address-use-after-scope)
#     target_link_options(tph_poisson_asan INTERFACE -fsanitize=address,undefined)
#   # elseif(MSVC) 
#   #   target_compile_options(tph_poisson_asan INTERFACE "-fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope")
#   #   target_link_options(tph_poisson_asan INTERFACE "-fsanitize=address")
#   endif()
# endif()

# add_library(tph_poisson_f STATIC
#   "tph_poisson_f.c")
# target_link_libraries(tph_poisson_f PUBLIC thinks::tph_poisson thinks::tph_poisson_asan)
# target_compile_options(tph_poisson_f PRIVATE ${_c_warnings})
# set_target_properties(tph_poisson_f PROPERTIES
#   C_STANDARD ${_c_standard}
#   C_STANDARD_REQUIRED ON
#   C_EXTENSIONS OFF
# )
# add_library(thinks::tph_poisson_f ALIAS tph_poisson_f)

# add_library(tph_poisson_d STATIC
#   "tph_poisson_d.c")
# target_link_libraries(tph_poisson_d PUBLIC thinks::tph_poisson thinks::tph_poisson_asan)
# target_compile_options(tph_poisson_d PRIVATE ${_c_warnings})
# set_target_properties(tph_poisson_d PROPERTIES
#   C_STANDARD ${_c_standard}
#   C_STANDARD_REQUIRED ON
#   C_EXTENSIONS OFF
# )
# add_library(thinks::tph_poisson_d ALIAS tph_poisson_d)

# # Fetch rpmalloc repo, source files are included directly in our test build, since the 
# # project does not have a CMake build system.
# FetchContent_Declare(rpmalloc
#   GIT_REPOSITORY https://github.com/mjansson/rpmalloc.git
#   GIT_TAG        1.4.5
#   GIT_SHALLOW    TRUE
#   GIT_PROGRESS   TRUE
# )
# FetchContent_GetProperties(rpmalloc)
# if(NOT rpmalloc_POPULATED)
#   FetchContent_Populate(rpmalloc)
# endif()

# # float
# add_executable(tph_poisson_test 
#   "test.cpp"
#   "${rpmalloc_SOURCE_DIR}/rpmalloc/rpmalloc.c")
# target_link_libraries(tph_poisson_test PRIVATE thinks::tph_poisson_f)
# target_include_directories(tph_poisson_test PRIVATE ${rpmalloc_SOURCE_DIR})
# target_compile_definitions(tph_poisson_test PRIVATE ENABLE_STATISTICS=1 ENABLE_ASSERTS=1)
# set_target_properties(tph_poisson_test PROPERTIES
#   CXX_STANDARD ${_cxx_standard}
#   CXX_STANDARD_REQUIRED ON
#   CXX_EXTENSIONS OFF
# )
# add_test(NAME tph_poisson_test COMMAND tph_poisson_test)

# # double
# add_executable(tph_poisson_test_d
#   "test.cpp"
#   "${rpmalloc_SOURCE_DIR}/rpmalloc/rpmalloc.c")
# target_link_libraries(tph_poisson_test_d PRIVATE thinks::tph_poisson_d)
# target_include_directories(tph_poisson_test_d PRIVATE ${rpmalloc_SOURCE_DIR})
# target_compile_definitions(tph_poisson_test_d PRIVATE TPH_POISSON_TEST_USE_DOUBLE ENABLE_STATISTICS=1 ENABLE_ASSERTS=1)
# set_target_properties(tph_poisson_test_d PROPERTIES
#   CXX_STANDARD ${_cxx_standard}
#   CXX_STANDARD_REQUIRED ON
#   CXX_EXTENSIONS OFF
# )
# add_test(NAME tph_poisson_test_d COMMAND tph_poisson_test_d)

# add_executable(tph_poisson_vec_test 
#   "tph_poisson_vec_test.c")
# target_link_libraries(tph_poisson_vec_test PRIVATE thinks::tph_poisson)
# if(NOT MSVC)
#   target_link_libraries(tph_poisson_vec_test PRIVATE m)
# endif()
# set_target_properties(tph_poisson_vec_test PROPERTIES
#   C_STANDARD ${_c_standard}
#   C_STANDARD_REQUIRED ON
#   C_EXTENSIONS OFF
# )
# add_test(NAME tph_poisson_vec_test COMMAND tph_poisson_vec_test)

# unset(_cxx_standard)
# unset(_c_standard)
# unset(_c_warnings)

# #
# # add_executable(tphvec_test 
# #   "tphvec.h" 
# #   "tphvec.c" 
# #   "tphvec_test.c")
# # # target_link_libraries(tph_poisson_test PRIVATE thinks::tph_poisson)
# # set_target_properties(tph_poisson_test PROPERTIES
# #   C_STANDARD ${_c_standard}
# #   C_STANDARD_REQUIRED ON
# #   C_EXTENSIONS OFF
# # )
# # add_test(NAME tphvec_test COMMAND tphvec_test)
