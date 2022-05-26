set(_tph_test_cmake_list_file ${CMAKE_CURRENT_LIST_FILE})

##
## Detect standard support.
##

# C++11 is the minimum required.
set(compiler_supports_cpp_11 TRUE)

foreach(feature ${CMAKE_CXX_COMPILE_FEATURES})
  if (${feature} STREQUAL cxx_std_14)
    set(compiler_supports_cpp_14 TRUE)
  elseif (${feature} STREQUAL cxx_std_17)
    set(compiler_supports_cpp_17 TRUE)
  elseif (${feature} STREQUAL cxx_std_20)
    set(compiler_supports_cpp_20 TRUE)
  elseif (${feature} STREQUAL cxx_std_23)
    set(compiler_supports_cpp_23 TRUE)
  endif()
endforeach()

##
## Test functions.
##

#############################################################################
# tph_test_set_test_options(
#     all|<tests>
#     [CXX_STANDARDS all|<args>...]
#     [COMPILE_DEFINITIONS <args>...]
#     [COMPILE_FEATURES <args>...]
#     [COMPILE_OPTIONS <args>...]
#     [LINK_LIBRARIES <args>...]
#     [LINK_OPTIONS <args>...])
#
# Supply test- and standard-specific build settings.
# Specify multiple tests using a list e.g., "test-foo;test-bar".
#
# Must be called BEFORE the test is created.
#############################################################################

function(tph_test_set_test_options tests)
  cmake_parse_arguments(args "" ""
    "CXX_STANDARDS;COMPILE_DEFINITIONS;COMPILE_FEATURES;COMPILE_OPTIONS;LINK_LIBRARIES;LINK_OPTIONS"
    ${ARGN})

  if(NOT args_CXX_STANDARDS)
    set(args_CXX_STANDARDS "all")
  endif()

  foreach(test ${tests})
    if("${test}" STREQUAL "all")
        set(test "")
    endif()

    foreach(cxx_standard ${args_CXX_STANDARDS})
      if("${cxx_standard}" STREQUAL "all")
        if("${test}" STREQUAL "")
            message(FATAL_ERROR "Not supported. Change defaults in: ${_tph_test_cmake_list_file}")
        endif()
        set(test_interface _tph_test_interface_${test})
      else()
        set(test_interface _tph_test_interface_${test}_cpp_${cxx_standard})
      endif()

      if(NOT TARGET ${test_interface})
        add_library(${test_interface} INTERFACE)
      endif()

      target_compile_definitions(${test_interface} INTERFACE ${args_COMPILE_DEFINITIONS})
      target_compile_features(${test_interface} INTERFACE ${args_COMPILE_FEATURES})
      target_compile_options(${test_interface} INTERFACE ${args_COMPILE_OPTIONS})
      target_link_libraries (${test_interface} INTERFACE ${args_LINK_LIBRARIES})
      target_link_options(${test_interface} INTERFACE ${args_LINK_OPTIONS})
    endforeach()
  endforeach()
endfunction()

# For internal use by tph_test_add_test_for().
function(_tph_test_add_test test_name file main cxx_standard)
  set(test_target ${test_name}_cpp${cxx_standard})

  if(TARGET ${test_target})
    message(FATAL_ERROR "Target ${test_target} has already been added.")
  endif()

  add_executable(${test_target} ${file})
  target_link_libraries(${test_target} PRIVATE ${main})

  # Set and require C++ standard.
  set_target_properties(${test_target} PROPERTIES
    CXX_STANDARD ${cxx_standard}
    CXX_STANDARD_REQUIRED ON
  )

  # Apply standard-specific build settings.
  if(TARGET _tph_test_interface__cpp_${cxx_standard})
    target_link_libraries(${test_target} 
      PRIVATE 
        _tph_test_interface__cpp_${cxx_standard}
    )
  endif()

  # Apply test-specific build settings.
  if(TARGET _tph_test_interface_${test_name})
    target_link_libraries(${test_target} 
      PRIVATE 
        _tph_test_interface_${test_name}
    )
  endif()

  # Apply test- and standard-specific build settings.
  if(TARGET _tph_test_interface_${test_name}_cpp_${cxx_standard})
    target_link_libraries(${test_target} 
      PRIVATE
        _tph_test_interface_${test_name}_cpp_${cxx_standard}
    )
  endif()

  # if (JSON_FastTests)
  #   add_test(NAME ${test_target}
  #     COMMAND ${test_target} ${DOCTEST_TEST_FILTER}
  #     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  #   )
  # else()
  add_test(NAME ${test_target}
    COMMAND ${test_target} #${DOCTEST_TEST_FILTER} --no-skip
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  # endif()
  set_tests_properties(${test_target} 
    PROPERTIES 
      LABELS "all" 
      # FIXTURES_REQUIRED TEST_DATA
  )

  # if(JSON_Valgrind)
  #     add_test(NAME ${test_target}_valgrind
  #         COMMAND ${memcheck_command} $<TARGET_FILE:${test_target}> ${DOCTEST_TEST_FILTER}
  #         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  #     )
  #     set_tests_properties(${test_target}_valgrind PROPERTIES
  #         LABELS "valgrind" FIXTURES_REQUIRED TEST_DATA
  #     )
  # endif()
endfunction()

#############################################################################
# tph_test_add_test_for(
#     <file>
#     MAIN <main>
#     [CXX_STANDARDS <version_number>...] [FORCE])
#
# Given a <file> unit-foo.cpp, produces
#
#     test-foo_cpp<version_number>
#
# if C++ standard <version_number> is supported by the compiler and the
# source file contains JSON_HAS_CPP_<version_number>.
# Use FORCE to create the test regardless of the file containing
# JSON_HAS_CPP_<version_number>.
# Test targets are linked against <main>.
# CXX_STANDARDS defaults to "11".
#############################################################################

function(tph_test_add_test_for file)
  cmake_parse_arguments(args "FORCE" "MAIN" "CXX_STANDARDS" ${ARGN})

  get_filename_component(file_basename ${file} NAME_WE)
  string(REGEX REPLACE "unit-([^$]+)" "test-\\1" test_name ${file_basename})

  if("${args_MAIN}" STREQUAL "")
    message(FATAL_ERROR "Required argument MAIN <main> missing.")
  endif()

  if("${args_CXX_STANDARDS}" STREQUAL "")
    set(args_CXX_STANDARDS 11)
  endif()

  # file(READ ${file} file_content)
  foreach(cxx_standard ${args_CXX_STANDARDS})
    if(NOT compiler_supports_cpp_${cxx_standard})
      continue()
    endif()

    # Add unconditionally if C++11 (default) or forced.
    if(NOT ("${cxx_standard}" STREQUAL 11 OR args_FORCE))
    #   string(FIND "${file_content}" JSON_HAS_CPP_${cxx_standard} has_cpp_found)
    #   if(${has_cpp_found} EQUAL -1)
    #     continue()
    #   endif()
    endif()

    _tph_test_add_test(${test_name} ${file} ${args_MAIN} ${cxx_standard})
  endforeach()
endfunction()