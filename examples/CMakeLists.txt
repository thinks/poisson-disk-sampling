# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(example_srcs
	json_example.cc
	simple_example.cc
	vec_traits_in_namespace_example.cc
	vec_traits_passed_in_example.cc
)

add_executable(thinks_poisson_disk_sampling_examples)
target_sources(thinks_poisson_disk_sampling_examples 
  PRIVATE 
    main.cc 
    ${example_srcs}
)
target_include_directories(thinks_poisson_disk_sampling_examples 
  PRIVATE
    .
    ../include/
)
#set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-internal-isystem ")    
target_include_directories(thinks_poisson_disk_sampling_examples SYSTEM 
  PRIVATE 
    external/json/single_include)

target_compile_options(thinks_poisson_disk_sampling_examples PRIVATE
  # Clang warnings.
  $<$<CXX_COMPILER_ID:Clang>:
    -Wall 
    -Wextra 
    -Wconversion 
    -Wsign-conversion 
    -Wshadow 
    -Wno-c++98-compat 
    -Wno-c++98-compat-pedantic 
    -Werror >
  # GCC warnings.
  $<$<CXX_COMPILER_ID:GNU>:
    -Wall -Wextra -Wshadow -Werror >
  # MSVC warnings.
  $<$<CXX_COMPILER_ID:MSVC>:
    /W4 /WX >)

set_target_properties(thinks_poisson_disk_sampling_examples PROPERTIES CXX_STANDARD 11)
