# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

thinks_cc_library(
  NAME
    poisson_disk_sampling
  HDRS
    "poisson_disk_sampling.h"
  COPTS
    ${THINK_DEFAULT_COPTS}
  PUBLIC
)

if (BUILD_TESTING)
  add_subdirectory(test)
  add_subdirectory(examples)
endif()
