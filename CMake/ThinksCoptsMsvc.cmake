# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# Flags inspired by:
# - [abseil] (https://github.com/abseil/abseil-cpp/blob/master/absl/copts/GENERATED_AbseilCopts.cmake)
# - [cpp-bp] (https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md)
# 
# Each flag is tagged with the source from which it is taken.

list(APPEND THINKS_MSVC_EXCEPTION_FLAGS
  # [abseil]
  "/U_HAS_EXCEPTIONS"  
  "/D_HAS_EXCEPTIONS=1" 
  "/EHsc"  # catches C++ exceptions only and tells the compiler to assume 
           # that functions declared as extern "C" never throw a C++ exception
)

list(APPEND THINKS_MSVC_FLAGS
  "/permissive-"  # [cpp-bp] enforces standards conformance
  
  "/Zc:__cplusplus"

  "/W4"  # [cpp-bp] warning level 4 (Abseil uses /W3)
  "/WX"  # [cpp-bp] warnings as errors
  "/w14640"  # [cpp-bp] 'instance' : construction of local static object is not thread-safe

  # The following should be considered according to [cpp-bp]: 
  #
  # "/w14242"  # [cpp-bp] 'identifier': conversion from 'type1' to 'type1', possible loss of data
  # "/w14254"  # [cpp-bp] 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
  # "/w14263"  # [cpp-bp] 'function': member function does not override any base class virtual member function
  # "/w14265"  # [cpp-bp] 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be destructed correctly
  # "/w14287"  # [cpp-bp] 'operator': unsigned/negative constant mismatch
  # "/w14289"  # [cpp-bp] nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
  # "/w14296"  # [cpp-bp] 'operator': expression is always 'boolean_value'
  # "/w14311"  # [cpp-bp] 'variable': pointer truncation from 'type1' to 'type2'
  # "/w14545"  # [cpp-bp] expression before comma evaluates to a function which is missing an argument list
  # "/w14546"  # [cpp-bp] function call before comma missing argument list
  # "/w14547"  # [cpp-bp] 'operator': operator before comma has no effect; expected operator with side-effect
  # "/w14549"  # [cpp-bp] 'operator': operator before comma has no effect; did you intend 'operator'?
  # "/w14555"  # [cpp-bp] expression has no effect; expected expression with side-effect
  # "/w14619"  # [cpp-bp] pragma warning: there is no warning number 'number'
  # "/w14826"  # [cpp-bp] Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
  # "/w14905"  # [cpp-bp] wide string literal cast to 'LPSTR'
  # "/w14906"  # [cpp-bp] string literal cast to 'LPWSTR'
  # "/w14928"  # [cpp-bp] illegal copy-initialization; more than one user-defined conversion has been implicitly applied  

  # Disabled warnings.
  "/wd4005"  # [abseil] 'identifier' : macro redefinition
  "/wd4068"  # [abseil] unknown pragma
  "/wd4180"  # [abseil] qualifier applied to function type has no meaning; ignored
  #"/wd4244"  # [abseil] 'conversion' conversion from 'type1' to 'type2', possible loss of data
  "/wd4267"  # [abseil] 'var' : conversion from 'size_t' to 'type', possible loss of data
  "/wd4503"  # [abseil] 'identifier' : decorated name length exceeded, name was truncated
  "/wd4800"  # [abseil] implicit conversion from 'type' to bool. Possible information loss

  # [abseil]
  "/DNOMINMAX"
  "/DWIN32_LEAN_AND_MEAN"
  "/D_CRT_SECURE_NO_WARNINGS"
  "/D_SCL_SECURE_NO_WARNINGS"
  "/D_ENABLE_EXTENDED_ALIGNED_STORAGE"
)

list(APPEND THINKS_MSVC_LINKOPTS
  "-ignore:4221"  # [abseil] this object file does not define any previously undefined public symbols
)

list(APPEND THINKS_MSVC_TEST_FLAGS
  "/wd4018"  # [abseil] 'expression' : signed/unsigned mismatch
  "/wd4101"  # [abseil] 'identifier' : unreferenced local variable
  "/wd4503"  # [abseil] 'identifier' : decorated name length exceeded, name was truncated
  "/wd4996"  # [abseil] the compiler encountered a deprecated declaration
  
  # [abseil]
  "/DNOMINMAX"  
)
