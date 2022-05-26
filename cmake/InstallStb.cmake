# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

##
## nothings::stb
##

FetchContent_Declare(
  nothings_stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        af1a5bc352164740c1cc1354942b1c6b72eacb8a
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)
FetchContent_GetProperties(nothings_stb)
if(NOT nothings_stb_POPULATED)
  FetchContent_Populate(nothings_stb)
endif()

# We need to manually create an "imported" target.
# This is a header-only library, and all the headers are in the same 
# folder, so we simply provide that as an include directory on our
# target.
add_library(stb INTERFACE)
target_include_directories(stb
  INTERFACE
    ${nothings_stb_SOURCE_DIR})
add_library(nothings::stb ALIAS stb)
