# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

thinks_cc_test(
  NAME
    poisson_disk_sampling_test
  SRCS
    "catch_main.cc"
    "poisson_disk_sampling_test.cc"
  COPTS
    ${THINKS_TEST_COPTS}
  DEPS
    thinks::poisson_disk_sampling
    Catch2::Catch2
    ${CMAKE_THREAD_LIBS_INIT}
)
