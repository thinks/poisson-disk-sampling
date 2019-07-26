# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.
#
# Inspired by Abseil (https://github.com/abseil/abseil-cpp).

# target_compile_options(thinks_poisson_disk_sampling_test PRIVATE
#  # Clang warnings.
#  $<$<CXX_COMPILER_ID:Clang>:
#    -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wno-c++98-compat -Werror >
#  # GCC warnings.
#  $<$<CXX_COMPILER_ID:GNU>:
#    -Wall -Wextra -Wshadow -Werror >
#  # MSVC warnings.
#  $<$<CXX_COMPILER_ID:MSVC>:
#    /W4 /WX >)

#target_compile_options(thinks_poisson_disk_sampling_examples PRIVATE
#  # Clang warnings.
#  $<$<CXX_COMPILER_ID:Clang>:
#    -Wall 
#    -Wextra 
#    -Wconversion 
#    -Wsign-conversion 
#    -Wshadow 
#    -Wno-c++98-compat 
#    -Wno-c++98-compat-pedantic 
#    -Werror 
#  >
#  # GCC warnings.
#  $<$<CXX_COMPILER_ID:GNU>:
#    -Wall 
#    -Wextra 
#    -Wshadow 
#    -Werror 
#  >
#  # MSVC warnings.
#  $<$<CXX_COMPILER_ID:MSVC>:
#    /W4 
#    /WX 
#  >)



list(APPEND THINKS_CLANG_CL_EXCEPTION_FLAGS
    "/U_HAS_EXCEPTIONS"
    "/D_HAS_EXCEPTIONS=1"
    "/EHsc"
)

list(APPEND THINKS_CLANG_CL_FLAGS
    "/W3"
    "-Wno-c++98-compat-pedantic"
    "-Wno-conversion"
    "-Wno-covered-switch-default"
    "-Wno-deprecated"
    "-Wno-disabled-macro-expansion"
    "-Wno-double-promotion"
    "-Wno-comma"
    "-Wno-extra-semi"
    "-Wno-extra-semi-stmt"
    "-Wno-packed"
    "-Wno-padded"
    "-Wno-sign-compare"
    "-Wno-float-conversion"
    "-Wno-float-equal"
    "-Wno-format-nonliteral"
    "-Wno-gcc-compat"
    "-Wno-global-constructors"
    "-Wno-exit-time-destructors"
    "-Wno-nested-anon-types"
    "-Wno-non-modular-include-in-module"
    "-Wno-old-style-cast"
    "-Wno-range-loop-analysis"
    "-Wno-reserved-id-macro"
    "-Wno-shorten-64-to-32"
    "-Wno-switch-enum"
    "-Wno-thread-safety-negative"
    "-Wno-unknown-warning-option"
    "-Wno-unreachable-code"
    "-Wno-unused-macros"
    "-Wno-weak-vtables"
    "-Wno-zero-as-null-pointer-constant"
    "-Wbitfield-enum-conversion"
    "-Wbool-conversion"
    "-Wconstant-conversion"
    "-Wenum-conversion"
    "-Wint-conversion"
    "-Wliteral-conversion"
    "-Wnon-literal-null-conversion"
    "-Wnull-conversion"
    "-Wobjc-literal-conversion"
    "-Wno-sign-conversion"
    "-Wstring-conversion"
    "/DNOMINMAX"
    "/DWIN32_LEAN_AND_MEAN"
    "/D_CRT_SECURE_NO_WARNINGS"
    "/D_SCL_SECURE_NO_WARNINGS"
    "/D_ENABLE_EXTENDED_ALIGNED_STORAGE"
)

list(APPEND THINKS_CLANG_CL_TEST_FLAGS
    "-Wno-c99-extensions"
    "-Wno-deprecated-declarations"
    "-Wno-missing-noreturn"
    "-Wno-missing-prototypes"
    "-Wno-missing-variable-declarations"
    "-Wno-null-conversion"
    "-Wno-shadow"
    "-Wno-shift-sign-overflow"
    "-Wno-sign-compare"
    "-Wno-unused-function"
    "-Wno-unused-member-function"
    "-Wno-unused-parameter"
    "-Wno-unused-private-field"
    "-Wno-unused-template"
    "-Wno-used-but-marked-unused"
    "-Wno-zero-as-null-pointer-constant"
    "-Wno-gnu-zero-variadic-macro-arguments"
)

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

list(APPEND THINKS_LLVM_EXCEPTION_FLAGS
    "-fexceptions"
)

list(APPEND THINKS_LLVM_FLAGS
    "-Wall"
    "-Wextra"
    "-Weverything"
    "-Wno-c++98-compat-pedantic"
    "-Wno-conversion"
    "-Wno-covered-switch-default"
    "-Wno-deprecated"
    "-Wno-disabled-macro-expansion"
    "-Wno-double-promotion"
    "-Wno-comma"
    "-Wno-extra-semi"
    "-Wno-extra-semi-stmt"
    "-Wno-packed"
    "-Wno-padded"
    "-Wno-sign-compare"
    "-Wno-float-conversion"
    "-Wno-float-equal"
    "-Wno-format-nonliteral"
    "-Wno-gcc-compat"
    "-Wno-global-constructors"
    "-Wno-exit-time-destructors"
    "-Wno-nested-anon-types"
    "-Wno-non-modular-include-in-module"
    "-Wno-old-style-cast"
    "-Wno-range-loop-analysis"
    "-Wno-reserved-id-macro"
    "-Wno-shorten-64-to-32"
    "-Wno-switch-enum"
    "-Wno-thread-safety-negative"
    "-Wno-unknown-warning-option"
    "-Wno-unreachable-code"
    "-Wno-unused-macros"
    "-Wno-weak-vtables"
    "-Wno-zero-as-null-pointer-constant"
    "-Wbitfield-enum-conversion"
    "-Wbool-conversion"
    "-Wconstant-conversion"
    "-Wenum-conversion"
    "-Wint-conversion"
    "-Wliteral-conversion"
    "-Wnon-literal-null-conversion"
    "-Wnull-conversion"
    "-Wobjc-literal-conversion"
    "-Wno-sign-conversion"
    "-Wstring-conversion"
)

list(APPEND THINKS_LLVM_TEST_FLAGS
    "-Wno-c99-extensions"
    "-Wno-deprecated-declarations"
    "-Wno-missing-noreturn"
    "-Wno-missing-prototypes"
    "-Wno-missing-variable-declarations"
    "-Wno-null-conversion"
    "-Wno-shadow"
    "-Wno-shift-sign-overflow"
    "-Wno-sign-compare"
    "-Wno-unused-function"
    "-Wno-unused-member-function"
    "-Wno-unused-parameter"
    "-Wno-unused-private-field"
    "-Wno-unused-template"
    "-Wno-used-but-marked-unused"
    "-Wno-zero-as-null-pointer-constant"
    "-Wno-gnu-zero-variadic-macro-arguments"
)

list(APPEND THINKS_MSVC_EXCEPTION_FLAGS
    "/U_HAS_EXCEPTIONS"
    "/D_HAS_EXCEPTIONS=1"
    "/EHsc"
)

list(APPEND THINKS_MSVC_FLAGS
    "/W3"
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




set(THINKS_LSAN_LINKOPTS "")
set(THINKS_HAVE_LSAN OFF)
set(THINKS_DEFAULT_LINKOPTS "")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(THINKS_DEFAULT_COPTS "${THINKS_GCC_FLAGS}")
  set(THINKS_TEST_COPTS "${THINKS_GCC_FLAGS};${THINKS_GCC_TEST_FLAGS}")
  set(THINKS_EXCEPTIONS_FLAG "${THINKS_GCC_EXCEPTION_FLAGS}")
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  # MATCHES so we get both Clang and AppleClang
  if(MSVC)
    # clang-cl is half MSVC, half LLVM
    set(THINKS_DEFAULT_COPTS "${THINKS_CLANG_CL_FLAGS}")
    set(THINKS_TEST_COPTS "${THINKS_CLANG_CL_FLAGS};${THINKS_CLANG_CL_TEST_FLAGS}")
    set(THINKS_EXCEPTIONS_FLAG "${THINKS_CLANG_CL_EXCEPTION_FLAGS}")
    set(THINKS_DEFAULT_LINKOPTS "${THINKS_MSVC_LINKOPTS}")
  else()
    set(THINKS_DEFAULT_COPTS "${THINKS_LLVM_FLAGS}")
    set(THINKS_TEST_COPTS "${THINKS_LLVM_FLAGS};${THINKS_LLVM_TEST_FLAGS}")
    set(THINKS_EXCEPTIONS_FLAG "${THINKS_LLVM_EXCEPTION_FLAGS}")
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      # AppleClang doesn't have lsan
      # https://developer.apple.com/documentation/code_diagnostics
      if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.5)
        set(THINKS_LSAN_LINKOPTS "-fsanitize=leak")
        set(THINKS_HAVE_LSAN ON)
      endif()
    endif()
  endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(THINKS_DEFAULT_COPTS "${THINKS_MSVC_FLAGS}")
  set(THINKS_TEST_COPTS "${THINKS_MSVC_FLAGS};${THINKS_MSVC_TEST_FLAGS}")
  set(THINKS_EXCEPTIONS_FLAG "${THINKS_MSVC_EXCEPTION_FLAGS}")
  set(THINKS_DEFAULT_LINKOPTS "${THINKS_MSVC_LINKOPTS}")
else()
  message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER}.  Building with no default flags")
  set(THINKS_DEFAULT_COPTS "")
  set(THINKS_TEST_COPTS "")
  set(THINKS_EXCEPTIONS_FLAG "")
endif()

if("${CMAKE_CXX_STANDARD}" EQUAL 98)
  message(FATAL_ERROR "Thinks requires at least C++11")
elseif(NOT "${CMAKE_CXX_STANDARD}")
  message(STATUS "Thinks: No CMAKE_CXX_STANDARD set, assuming 11")
  set(THINKS_CXX_STANDARD 11)
else()
  set(THINKS_CXX_STANDARD "${CMAKE_CXX_STANDARD}")
endif()