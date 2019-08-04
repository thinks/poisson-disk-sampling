# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

list(APPEND THINKS_MSVC_EXCEPTION_FLAGS
    "/U_HAS_EXCEPTIONS"
    "/D_HAS_EXCEPTIONS=1"
    "/EHsc"
)

list(APPEND THINKS_MSVC_FLAGS
    "/W4"
    "/WX"
    "/DNOMINMAX"
    "/DWIN32_LEAN_AND_MEAN"
    "/D_CRT_SECURE_NO_WARNINGS"
    "/D_SCL_SECURE_NO_WARNINGS"
    "/D_ENABLE_EXTENDED_ALIGNED_STORAGE"
    "/wd4005"
    "/wd4068"
    "/wd4180"
    "/wd4244"
    "/wd4267"
    "/wd4503"
    "/wd4800"
)

list(APPEND THINKS_MSVC_LINKOPTS
    "-ignore:4221"
)

list(APPEND THINKS_MSVC_TEST_FLAGS
    "/wd4018"
    "/wd4101"
    "/wd4503"
    "/wd4996"
    "/DNOMINMAX"
)
