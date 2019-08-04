# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# TESTONLY?
thinks_cc_library(
  NAME
    catch_utils
  HDRS
    "catch_utils.h"
  SRCS
    "catch_utils.cc"
  COPTS
    ${THINKS_DEFAULT_COPTS}
  DEPS
    Catch2::Catch2
  TESTONLY
)

thinks_cc_test(
  NAME
    poisson_disk_sampling_test
  SRCS
    "catch_main.cc"
    "poisson_disk_sampling_test.cc"
  COPTS
    ${THINKS_TEST_COPTS}
  DEPS
    thinks::catch_utils
    Catch2::Catch2
)
