# Copyright (C) 2018 Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(tests
	poisson_disk_sampling_test.cc
)

add_executable(thinks_poisson_disk_sampling_test)
target_sources(thinks_poisson_disk_sampling_test 
  PRIVATE 
    main.cc 
    ${tests})
target_include_directories(thinks_poisson_disk_sampling_test 
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/
  SYSTEM PRIVATE
    external/)
set_target_properties(thinks_poisson_disk_sampling_test PROPERTIES CXX_STANDARD 11) 

target_compile_options(thinks_poisson_disk_sampling_test PRIVATE
  # GCC/clang warnings.
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
    -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wno-c++98-compat -Werror >
  # MSVC warnings.
  $<$<CXX_COMPILER_ID:MSVC>:
    /W4 /WX >)

add_test(NAME test COMMAND thinks_poisson_disk_sampling_test)
