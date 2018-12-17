# Copyright (C) 2018 Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(examples
	json_example.cc
	simple_example.cc
	vec_traits_in_namespace_example.cc
	vec_traits_passed_in_example.cc
)

add_executable(thinks_poisson_disk_sampling_examples "")
target_sources(thinks_poisson_disk_sampling_examples 
  PRIVATE 
    main.cc 
    ${examples})
target_include_directories(thinks_poisson_disk_sampling_examples 
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/
  SYSTEM PRIVATE
    external/json/single_include)
set_target_properties(thinks_poisson_disk_sampling_examples PROPERTIES CXX_STANDARD 11)

target_compile_options(thinks_poisson_disk_sampling_test PRIVATE
  # GCC/clang warnings.
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
    -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wno-c++98-compat -Werror >
  # MSVC warnings.
  $<$<CXX_COMPILER_ID:MSVC>:
    /W4 /WX >)
