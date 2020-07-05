# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.
#
# Inspired by Abseil (https://github.com/abseil/abseil-cpp).

include(CMakeParseArguments)
include(ThinksConfigureCopts)

# The IDE folder for 'thinks' that will be used if 'thinks' is 
# included in a CMake project that sets
#   set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# For example, Visual Studio supports folders.
set(THINKS_IDE_FOLDER thinks)

# thinks_cc_library()
#
# CMake function to imitate Bazel's cc_library rule.
#
# Parameters:
# NAME: name of target (see Note)
# HDRS: List of public header files for the library
# SRCS: List of source files for the library
# DEPS: List of other libraries to be linked in to the binary targets
# COPTS: List of private compile options
# DEFINES: List of public defines
# LINKOPTS: List of link options
# PUBLIC: Add this so that this library will be exported under thinks::
# Also in IDE, target will appear in 'thinks' folder while non-PUBLIC 
# will be in thinks/internal.
#
# Note:
# By default, thinks_cc_library will always create a library 
# named thinks_${NAME}, and alias target thinks::${NAME}. The 
# thinks:: form should always be used. This is to reduce 
# namespace pollution.
#
# thinks_cc_library(
#   NAME
#     awesome
#   HDRS
#     "a.h"
#   SRCS
#     "a.cc"
#   COPTS
#     ${THINKS_DEFAULT_COPTS}
# )
#
# thinks_cc_library(
#   NAME
#     fantastic_lib
#   SRCS
#     "b.cc"
#   COPTS
#     ${THINKS_DEFAULT_COPTS}
#   DEPS
#     thinks::awesome  (not "awesome"!)
#   PUBLIC
# )
#
# thinks_cc_library(
#   NAME
#     main_lib
#   ...
#   DEPS
#     thinks::fantastic_lib
# )
function(thinks_cc_library)
  cmake_parse_arguments(THINKS_CC_LIB
    "PUBLIC;TESTONLY"
    "NAME"
    "HDRS;SRCS;COPTS;DEFINES;LINKOPTS;DEPS"
    ${ARGN}
  )

  if(NOT THINKS_CC_LIB_TESTONLY OR BUILD_TESTING)
    set(_NAME "thinks_${THINKS_CC_LIB_NAME}")

    # Check if this is a header-only library.
    set(THINKS_CC_SRCS "${THINKS_CC_LIB_SRCS}")
    foreach(src_file IN LISTS THINKS_CC_SRCS)
      if(${src_file} MATCHES ".*\\.(h|inc)")
        list(REMOVE_ITEM THINKS_CC_SRCS "${src_file}")
      endif()
    endforeach()
    if("${THINKS_CC_SRCS}" STREQUAL "")
      set(THINKS_CC_LIB_IS_INTERFACE 1)
    else()
      set(THINKS_CC_LIB_IS_INTERFACE 0)
    endif()

    if(NOT THINKS_CC_LIB_IS_INTERFACE)
      # Library with at least one non-header file.
      add_library(${_NAME} STATIC "")
      target_sources(${_NAME} 
        PRIVATE 
          ${THINKS_CC_LIB_SRCS} 
          ${THINKS_CC_LIB_HDRS}
      )
      target_include_directories(${_NAME}
        PUBLIC
          ${THINKS_COMMON_INCLUDE_DIRS}
      )
      target_compile_options(${_NAME}
        PRIVATE 
          ${THINKS_CC_LIB_COPTS}
      )
      target_link_libraries(${_NAME}
        PUBLIC 
          ${THINKS_CC_LIB_DEPS}
        PRIVATE
          ${THINKS_CC_LIB_LINKOPTS}
          ${THINKS_DEFAULT_LINKOPTS}
      )
      target_compile_definitions(${_NAME} 
        PUBLIC 
          ${THINKS_CC_LIB_DEFINES}
      )

      # Add all 'thinks' targets to a a folder in the IDE for organization.
      if(THINKS_CC_LIB_PUBLIC)
        set_property(TARGET ${_NAME} PROPERTY FOLDER ${THINKS_IDE_FOLDER})
      elseif(THINKS_CC_LIB_TESTONLY)
        set_property(TARGET ${_NAME} PROPERTY FOLDER ${THINKS_IDE_FOLDER}/test)  
      else()
        set_property(TARGET ${_NAME} PROPERTY FOLDER ${THINKS_IDE_FOLDER}/internal)
      endif()

      # Only non-INTERFACE libraries have the CXX_STANDARD property set.
      set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD ${THINKS_CXX_STANDARD})
      set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
    else()
      # Generating header-only library.
      add_library(${_NAME} INTERFACE)
      target_include_directories(${_NAME}
        INTERFACE
          ${THINKS_COMMON_INCLUDE_DIRS}
      )
      target_link_libraries(${_NAME}
        INTERFACE
          ${THINKS_CC_LIB_DEPS}
          ${THINKS_CC_LIB_LINKOPTS}
          ${THINKS_DEFAULT_LINKOPTS}
      )
      target_compile_definitions(${_NAME} 
        INTERFACE 
          ${THINKS_CC_LIB_DEFINES}
      )
    endif()

    add_library(thinks::${THINKS_CC_LIB_NAME} ALIAS ${_NAME})
  endif()
endfunction()


# thinks_cc_executable
#
function(thinks_cc_executable)
  cmake_parse_arguments(THINKS_CC_EXE
    ""
    "NAME"
    "HDRS;SRCS;COPTS;DEFINES;LINKOPTS;DEPS"
    ${ARGN}
  )

  set(_NAME "thinks_${THINKS_CC_EXE_NAME}")

  add_executable(${_NAME} "")
  target_sources(${_NAME} 
    PRIVATE 
      ${THINKS_CC_EXE_SRCS} 
      ${THINKS_CC_EXE_HDRS}
  )

  # NOTE(thinks): 
  #   Include directories for an executable should be set using 
  #   library dependencies. Those dependencies should set up
  #   the include paths to their headers correctly.

  target_compile_options(${_NAME}
    PRIVATE 
      ${THINKS_CC_EXE_COPTS}
  )
  target_link_libraries(${_NAME}
    PUBLIC 
      ${THINKS_CC_EXE_DEPS}
    PRIVATE
      ${THINKS_CC_EXE_LINKOPTS}
      ${THINKS_DEFAULT_LINKOPTS}
  )
  target_compile_definitions(${_NAME} 
    PUBLIC 
      ${THINKS_CC_EXE_DEFINES}
  )

  # Add all 'thinks' targets to a a folder in the IDE for organization.
  set_property(TARGET ${_NAME} PROPERTY FOLDER ${THINKS_IDE_FOLDER})

  set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD ${THINKS_CXX_STANDARD})
  set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

  add_executable(thinks::${THINKS_CC_EXE_NAME} ALIAS ${_NAME})
endfunction()


# thinks_cc_test()
#
# CMake function to imitate Bazel's cc_test rule.
#
# Parameters:
# NAME: name of target (see Usage below)
# SRCS: List of source files for the binary
# DEPS: List of other libraries to be linked in to the binary targets
# COPTS: List of private compile options
# DEFINES: List of public defines
# LINKOPTS: List of link options
#
# Note:
# By default, thinks_cc_test will always create a binary named thinks_${NAME}.
# This will also add it to ctest list as thinks_${NAME}.
#
# Usage:
# thinks_cc_library(
#   NAME
#     awesome
#   HDRS
#     "a.h"
#   SRCS
#     "a.cc"
#   PUBLIC
# )
#
# thinks_cc_test(
#   NAME
#     awesome_test
#   SRCS
#     "awesome_test.cc"
#   DEPS
#     thinks::awesome
#     Catch2::Catch2  (Or some other nice testing framework!)
# )
function(THINKS_CC_TEST)
  if(NOT BUILD_TESTING)
    return()
  endif()

  cmake_parse_arguments(THINKS_CC_TEST
    ""
    "NAME"
    "SRCS;COPTS;DEFINES;LINKOPTS;DEPS"
    ${ARGN}
  )

  set(_NAME "thinks_${THINKS_CC_TEST_NAME}")
  add_executable(${_NAME} "")
  target_sources(${_NAME} 
    PRIVATE 
      ${THINKS_CC_TEST_SRCS})
  target_include_directories(${_NAME}
    PUBLIC 
      ${THINKS_COMMON_INCLUDE_DIRS}
  )
  target_compile_definitions(${_NAME}
    PUBLIC 
      ${THINKS_CC_TEST_DEFINES}
  )
  target_compile_options(${_NAME}
    PRIVATE 
      ${THINKS_CC_TEST_COPTS}
  )
  target_link_libraries(${_NAME}
    PUBLIC 
      ${THINKS_CC_TEST_DEPS}
    PRIVATE 
      ${THINKS_CC_TEST_LINKOPTS}
  )

  # Add all Thinks targets to a folder in the IDE for organization.
  set_property(TARGET ${_NAME} PROPERTY FOLDER ${THINKS_IDE_FOLDER}/test)

  set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD ${THINKS_CXX_STANDARD})
  set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

  add_test(NAME ${_NAME} COMMAND ${_NAME})
endfunction()

function(check_target my_target)
  if(NOT TARGET ${my_target})
    message(FATAL_ERROR " thinks: compilation requires a ${my_target} CMake target in your project.")
  endif()
endfunction()
