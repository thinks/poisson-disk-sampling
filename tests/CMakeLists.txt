cmake_minimum_required(VERSION 3.13)

# option(JSON_Valgrind    "Execute test suite with Valgrind." OFF)
# option(JSON_FastTests   "Skip expensive/slow tests." OFF)

# TODO?
# option(TPH_StaticTests  "Build static tests." ON)

set(TPH_TestStandards "" CACHE STRING "The list of standards to test explicitly.")

include(InstallCatch2)
include(test)

##
## Override standard support.
##

# Clang only supports C++17 starting from Clang 5.0.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
    unset(compiler_supports_cpp_17)
endif()
# MSVC 2015 (14.0) does not support C++17.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.1)
    unset(compiler_supports_cpp_17)
endif()

# Clang C++20 support appears insufficient prior to Clang 9.0.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    unset(compiler_supports_cpp_20)
endif()
# GCC started supporting C++20 features in 8.0 but seems incomplete prior to 9.0.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    unset(compiler_supports_cpp_20)
endif()

##
## test_main library with shared code to speed up build and common settings.
##

add_executable(unit_tests_main "src/unit_tests.cpp")
target_compile_features(unit_tests_main PRIVATE cxx_std_11)
target_compile_options(unit_tests_main 
  PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/EHsc;$<$<CONFIG:Release>:/Od>>
    # MSVC: Force to always compile with W4
    $<$<CXX_COMPILER_ID:MSVC>:/W4>

    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-deprecated;-Wno-float-equal>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations>
)
# target_include_directories(test_main 
#   PUBLIC
#     ${PROJECT_BINARY_DIR}/include # ???
# )

target_link_libraries(unit_tests_main 
  PRIVATE 
    Catch2::Catch2WithMain
    ${TPH_PDS_TARGET_NAME}
)

# Set and require C++ standard.
set_target_properties(unit_tests_main PROPERTIES
  CXX_STANDARD ${CMAKE_CXX_STANDARD}
  CXX_STANDARD_REQUIRED ON
)

add_test(NAME unit_tests_main COMMAND unit_tests_main)

# target_compile_definitions(test_main PUBLIC
#     DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#     JSON_TEST_KEEP_MACROS
# )


##
## Define test- and standard-specific build settings.
##

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.0
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0 AND NOT MINGW)
    # Link to std::file_system lib.
    tph_test_set_test_options(all CXX_STANDARDS 17 LINK_LIBRARIES stdc++fs)
endif()

# # Disable exceptions for test-disabled_exceptions.
# tph_test_set_test_options(test-disabled_exceptions COMPILE_DEFINITIONS JSON_NOEXCEPTION)
# if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#   tph_test_set_test_options(test-disabled_exceptions COMPILE_OPTIONS -fno-exceptions)
# elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
#     # disabled due to https://github.com/nlohmann/json/discussions/2824
#     #json_test_set_test_options(test-disabled_exceptions COMPILE_DEFINITIONS _HAS_EXCEPTIONS=0 COMPILE_OPTIONS /EH)
# endif()

##
## Add unit tests.
##

if("${TPH_TestStandards}" STREQUAL "")
    set(test_cxx_standards 11 14 17 20 23)
    unset(test_force)
else()
    set(test_cxx_standards ${TPH_TestStandards})
    set(test_force FORCE)
endif()

# Print selected standards marking unavailable ones with brackets
set(msg_standards "")
foreach(cxx_standard ${test_cxx_standards})
  if(compiler_supports_cpp_${cxx_standard})
    list(APPEND msg_standards ${cxx_standard})
  else()
    list(APPEND msg_standards [${cxx_standard}])
  endif()
endforeach()
string(JOIN " " msg_standards ${msg_standards})
set(msg "Testing standards: ${msg_standards}")
if(test_force)
  string(APPEND msg " (forced)")
endif()
message(STATUS "${msg}")

# *DO* use tph_test_set_test_options() above this line

file(GLOB files src/unit-*.cpp)
foreach(file ${files})
  tph_test_add_test_for(${file} MAIN test_main CXX_STANDARDS ${test_cxx_standards} ${test_force})
endforeach()

# *DO NOT* use tph_test_set_test_options() below this line





##
## Test the generated build configs.
##

# These tests depend on the generated file nlohmann_jsonConfig.cmake.
# if (TPH_Install)
#  add_subdirectory(cmake_import)
#  add_subdirectory(cmake_import_minver)
# endif()

# TODO(tohi): Enable!
#add_subdirectory(cmake_add_subdirectory)
#add_subdirectory(cmake_fetch_content)
#add_subdirectory(cmake_target_include_directories)