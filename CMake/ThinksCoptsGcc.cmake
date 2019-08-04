# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

list(APPEND THINKS_GCC_EXCEPTION_FLAGS
    "-fexceptions"
)

list(APPEND THINKS_GCC_FLAGS
    "-Wall"
    "-Wextra"
    "-Wcast-qual"
    "-Wconversion-null"
    "-Wmissing-declarations"
    "-Woverlength-strings"
    "-Wpointer-arith"
    "-Wunused-local-typedefs"
    "-Wunused-result"
    "-Wvarargs"
    "-Wvla"
    "-Wwrite-strings"
    "-Wno-missing-field-initializers"
    "-Wno-sign-compare"
)

list(APPEND THINKS_GCC_TEST_FLAGS
    "-Wno-conversion-null"
    "-Wno-deprecated-declarations"
    "-Wno-missing-declarations"
    "-Wno-sign-compare"
    "-Wno-unused-function"
    "-Wno-unused-parameter"
    "-Wno-unused-private-field"
)
