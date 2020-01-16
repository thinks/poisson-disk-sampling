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

  "-Werror"  # warnings as errors
  "-Wshadow"  # warn the user if a variable declaration shadows one from a parent context
  "-Wnon-virtual-dtor"  # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
  "-Wold-style-cast"  # warn for c-style casts
  "-Wcast-align"  # warn for potential performance problem casts
  "-Wunused"  # on anything being unused
  "-Woverloaded-virtual"  # warn if you overload (not override) a virtual function
  "-Wpedantic"  # (all versions of GCC, Clang >= 3.2) warn if non-standard C++ is used
  "-Wconversion"  # warn on type conversions that may lose data
  "-Wsign-conversion"  # (Clang all versions, GCC >= 4.3) warn on sign conversions
  "-Wmisleading-indentation"  # (only in GCC >= 6.0) warn if indentation implies blocks where blocks do not exist
  "-Wduplicated-cond"  # (only in GCC >= 6.0) warn if if / else chain has duplicated conditions
  "-Wduplicated-branches"  # (only in GCC >= 7.0) warn if if / else branches have duplicated code
  "-Wlogical-op"  # (only in GCC) warn about logical operations being used where bitwise were probably wanted
  #"-Wnull-dereference"  # (only in GCC >= 6.0) warn if a null dereference is detected
  #"-Wuseless-cast"  # (only in GCC >= 4.8) warn if you perform a cast to the same type
  "-Wdouble-promotion"  # (GCC >= 4.6, Clang >= 3.8) warn if float is implicit promoted to double
  "-Wformat=2"  # warn on security issues around functions that format output (ie printf)
  #"-Wlifetime"  # (only special branch of Clang currently) shows object lifetime issues    
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
