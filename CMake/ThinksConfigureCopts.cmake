# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.
#
# Inspired by Abseil (https://github.com/abseil/abseil-cpp).

include(ThinksCoptsClangCl)
include(ThinksCoptsGcc)
include(ThinksCoptsLlvm)
include(ThinksCoptsMsvc)

set(THINKS_LSAN_LINKOPTS "")
set(THINKS_HAVE_LSAN OFF)
set(THINKS_DEFAULT_LINKOPTS "")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(THINKS_DEFAULT_COPTS "${THINKS_GCC_FLAGS}")
  set(THINKS_TEST_COPTS "${THINKS_GCC_FLAGS};${THINKS_GCC_TEST_FLAGS}")
  set(THINKS_EXCEPTIONS_FLAG "${THINKS_GCC_EXCEPTION_FLAGS}")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  # MATCHES so we get both Clang and AppleClang.
  if(MSVC)
    # clang-cl is half MSVC, half LLVM.
    set(THINKS_DEFAULT_COPTS "${THINKS_CLANG_CL_FLAGS}")
    set(THINKS_TEST_COPTS "${THINKS_CLANG_CL_FLAGS};${THINKS_CLANG_CL_TEST_FLAGS}")
    set(THINKS_EXCEPTIONS_FLAG "${THINKS_CLANG_CL_EXCEPTION_FLAGS}")
    set(THINKS_DEFAULT_LINKOPTS "${THINKS_MSVC_LINKOPTS}")
  else()
    set(THINKS_DEFAULT_COPTS "${THINKS_LLVM_FLAGS}")
    set(THINKS_TEST_COPTS "${THINKS_LLVM_FLAGS};${THINKS_LLVM_TEST_FLAGS}")
    set(THINKS_EXCEPTIONS_FLAG "${THINKS_LLVM_EXCEPTION_FLAGS}")
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      # AppleClang doesn't have lsan.
      # https://developer.apple.com/documentation/code_diagnostics
      if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)
        set(THINKS_LSAN_LINKOPTS "-fsanitize=leak")
        set(THINKS_HAVE_LSAN ON)
      endif()
    endif()
  endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # Remove default warning level from CMAKE_CXX_FLAGS,
  # otherwise we get an annoying warning for overriding the warning level.
  #
  # NOTE(thinks): 
  #   For CMake 3.15 and above there seems to be a policy that removes
  #   the default warning levels. However, seems a bit aggressive to 
  #   require that version for now.
  string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

  set(THINKS_DEFAULT_COPTS "${THINKS_MSVC_FLAGS}")
  set(THINKS_TEST_COPTS "${THINKS_MSVC_FLAGS};${THINKS_MSVC_TEST_FLAGS}")
  set(THINKS_EXCEPTIONS_FLAG "${THINKS_MSVC_EXCEPTION_FLAGS}")
  set(THINKS_DEFAULT_LINKOPTS "${THINKS_MSVC_LINKOPTS}")
else()
  message(WARNING "thinks: Unknown compiler: ${CMAKE_CXX_COMPILER}. Building with no default flags")
  set(THINKS_DEFAULT_COPTS "")
  set(THINKS_TEST_COPTS "")
  set(THINKS_EXCEPTIONS_FLAG "")
endif()

if("${CMAKE_CXX_STANDARD}" EQUAL 98)
  message(FATAL_ERROR "thinks: requires at least C++14")
elseif(NOT "${CMAKE_CXX_STANDARD}")
  message(STATUS "thinks: No CMAKE_CXX_STANDARD set, assuming 17")
  set(THINKS_CXX_STANDARD 17)
else()
  set(THINKS_CXX_STANDARD "${CMAKE_CXX_STANDARD}")
endif()
