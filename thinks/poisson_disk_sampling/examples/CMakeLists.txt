# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

thinks_cc_executable(
  NAME
    poisson_disk_sampling_examples
  HDRS
    "json_example.h"
    "simple_example.h"
    "vec_traits_in_namespace_example.h"
    "vec_traits_passed_in_example.h"
  SRCS
    "json_example.cc"
    "main.cc"
    "simple_example.cc"
    "vec_traits_in_namespace_example.cc"
    "vec_traits_passed_in_example.cc"
  COPTS
    ${THINKS_DEFAULT_COPTS}
  DEPS
    thinks::poisson_disk_sampling
    nlohmann_json::nlohmann_json
)
