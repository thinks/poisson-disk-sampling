# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

thinks_cc_executable(
  NAME
    json_example
  SRCS
    "config.h"
    "json_example.cc"
  COPTS
    ${THINKS_DEFAULT_COPTS}
  DEPS
    thinks::poisson_disk_sampling
    nlohmann_json::nlohmann_json
)

thinks_cc_executable(
  NAME
    periodogram_example
  SRCS
    "config.h"
    "periodogram_example.cc"
  COPTS
    ${THINKS_DEFAULT_COPTS}
  DEPS
    thinks::poisson_disk_sampling
    thinks_pnm_io
    d1vanov::simple_fft
)

thinks_cc_executable(
  NAME
    simple_example
  SRCS
    "config.h"
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
    "config.h"
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
    "config.h"
    "vec_traits_passed_in_example.cc"
  COPTS
    ${THINKS_DEFAULT_COPTS}
  DEPS
    thinks::poisson_disk_sampling
)
