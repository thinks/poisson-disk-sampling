# Copyright (C) 2018 Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

set(tests
	poisson_disk_sampling_test.cc
)

add_executable(thinks_poisson_disk_sampling_test
	main.cc
	${tests})
target_include_directories(thinks_poisson_disk_sampling_test SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(thinks_poisson_disk_sampling_test PUBLIC thinks_poisson_disk_sampling)
set_target_properties(thinks_poisson_disk_sampling_test PROPERTIES CXX_STANDARD 11)

add_test(NAME test COMMAND thinks_poisson_disk_sampling_test)
