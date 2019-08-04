# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.5)
project(poisson_disk_sampling CXX)

message(STATUS "CMake version: ${CMAKE_VERSION}")

# if command can use IN_LIST
cmake_policy(SET CMP0057 NEW)

# For cmake > 3.15 don't add warning levels by default.
# Probably a little early to enable this for now.
# cmake_policy(SET CMP0092 NEW)

list(APPEND CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_LIST_DIR}/CMake
)

include(ThinksHelpers)

# include current path
list(APPEND THINKS_COMMON_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

# Build tests and examples only when _not_ included as subproject.
if("^${CMAKE_SOURCE_DIR}$" STREQUAL "^${PROJECT_SOURCE_DIR}$")
  # Enable CTest.  
  # This will set BUILD_TESTING to ON unless otherwise specified
  # on the command line.
  include(CTest)
  enable_testing()

  set(JSON_BuildTests OFF CACHE INTERNAL "")
  set(JSON_Install OFF CACHE INTERNAL "")
  add_subdirectory(external/json)
  check_target(nlohmann_json::nlohmann_json)
 
  add_subdirectory(external/Catch2)
  check_target(Catch2::Catch2)
endif()

add_subdirectory(thinks)
