cmake_minimum_required(VERSION 3.13)

# option(JSON_Valgrind    "Execute test suite with Valgrind." OFF)
# option(JSON_FastTests   "Skip expensive/slow tests." OFF)

# TODO?
# option(TPH_StaticTests  "Build static tests." ON)

include(InstallCatch2)

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