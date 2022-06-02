# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

##
## d1vanov::simple_fft
##

FetchContent_Declare(
  d1vanov_simple_fft
  GIT_REPOSITORY https://github.com/d1vanov/Simple-FFT.git
  GIT_TAG        a0cc843ff36d33ad09c08674b9503614742ad0b9
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)
FetchContent_GetProperties(d1vanov_simple_fft)
if(NOT d1vanov_simple_fft_POPULATED)
  FetchContent_Populate(d1vanov_simple_fft)
endif()

# We need to manually create an "imported" target.
# This is a header-only library, and all the headers are in the same 
# folder, so we simply provide that as an include directory on our
# target.
add_library(simple_fft INTERFACE)
target_include_directories(simple_fft
  INTERFACE
    ${d1vanov_simple_fft_SOURCE_DIR}/include)
add_library(d1vanov::simple_fft ALIAS simple_fft)
