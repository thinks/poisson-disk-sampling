# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

thinks_cc_executable(
  NAME
    json_example
  SRCS
    "json_example.cc"
  COPTS
    ${THINKS_DEFAULT_COPTS}
  DEPS
    thinks::poisson_disk_sampling
    nlohmann_json::nlohmann_json
)

thinks_cc_executable(
  NAME
    simple_example
  SRCS
    "simple_example.cc"
  COPTS
    ${THINKS_DEFAULT_COPTS}
  DEPS
    thinks::poisson_disk_sampling
)

thinks_cc_executable(
  NAME
    vec_traits_in_namespace_example
  SRCS
    "vec_traits_in_namespace_example.cc"
  COPTS
    ${THINKS_DEFAULT_COPTS}
  DEPS
    thinks::poisson_disk_sampling
)

thinks_cc_executable(
  NAME
    vec_traits_passed_in_example
  SRCS
    "vec_traits_passed_in_example.cc"
  COPTS
    ${THINKS_DEFAULT_COPTS}
  DEPS
    thinks::poisson_disk_sampling
)
