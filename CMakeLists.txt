# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.11)

# Set BUILD_TESTING to OFF by default.
# This must come before the project() and include(CTest) lines.
# option(BUILD_TESTING "Build tests" OFF)
# option(BUILD_STATIC_TESTING "Build static tests" OFF)
# option(WARNINGS_AS_ERRORS "Warnings as errors" OFF)

##
## PROJECT
## Name and version.
##
project(tph_poisson_disk_sampling VERSION 0.1.0 LANGUAGES CXX)
message(STATUS "CMake version: ${CMAKE_VERSION}")

##
## MAIN PROJECT CHECK
## Determine if tph_poisson_disk_sampling is built as a subproject (using add_subdirectory) 
## or if it is the main project.
##
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MAIN_PROJECT ON)
endif()

##
## INCLUDE
##
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(FetchContent)

##
## OPTIONS
##
if (POLICY CMP0077)
  # Allow CMake 3.13+ to override options when using FetchContent / add_subdirectory.
  cmake_policy(SET CMP0077 NEW)
endif ()

# VERSION_GREATER_EQUAL is not available in CMake 3.1
if(${MAIN_PROJECT} AND (${CMAKE_VERSION} VERSION_EQUAL 3.13 OR ${CMAKE_VERSION} VERSION_GREATER 3.13))
  set(TPH_BuildTests_INIT ON)
else()
  set(TPH_BuildTests_INIT OFF)
endif()

option(TPH_BuildTests     "Build the unit tests when BUILD_TESTING is enabled." ${TPH_BuildTests_INIT})
option(TPH_BuildExamples  "Build the examples." ${TPH_BuildTests_INIT})
option(TPH_CI             "Enable CI build targets." OFF)
option(TPH_Install        "Install CMake targets during install step." ${MAIN_PROJECT})
option(TPH_SystemInclude  "Include as system headers (skip for clang-tidy)." OFF)

if (TPH_CI) 
  include(ci)
endif()

## 
## CONFIGURATION
##
include(GNUInstallDirs)

set(TPH_PDS_TARGET_NAME                 ${PROJECT_NAME})
set(TPH_PDS_CONFIG_INSTALL_DIR          "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE INTERNAL "")
set(TPH_PDS_INCLUDE_BUILD_DIR           "${PROJECT_SOURCE_DIR}/include/")
set(TPH_PDS_INCLUDE_INSTALL_DIR         "${CMAKE_INSTALL_INCLUDEDIR}")
set(TPH_PDS_TARGETS_EXPORT_NAME         "${PROJECT_NAME}Targets")
set(TPH_PDS_CMAKE_CONFIG_TEMPLATE       "cmake/config.cmake.in")
set(TPH_PDS_CMAKE_CONFIG_DIR            "${CMAKE_CURRENT_BINARY_DIR}")
set(TPH_PDS_CMAKE_VERSION_CONFIG_FILE   "${TPH_PDS_CMAKE_CONFIG_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(TPH_PDS_CMAKE_PROJECT_CONFIG_FILE   "${TPH_PDS_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Config.cmake")
set(TPH_PDS_CMAKE_PROJECT_TARGETS_FILE  "${TPH_PDS_CMAKE_CONFIG_DIR}/${PROJECT_NAME}Targets.cmake")
set(TPH_PDS_PKGCONFIG_INSTALL_DIR       "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

if (TPH_SystemInclude)
  set(TPH_PDS_SYSTEM_INCLUDE "SYSTEM")
endif()

##
## TARGET
## Create target and add include path.
##
add_library(${TPH_PDS_TARGET_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${TPH_PDS_TARGET_NAME} ALIAS ${TPH_PDS_TARGET_NAME})
if (${CMAKE_VERSION} VERSION_LESS "3.8.0")
  target_compile_features(${TPH_PDS_TARGET_NAME} INTERFACE cxx_range_for)
else()
  target_compile_features(${TPH_PDS_TARGET_NAME} INTERFACE cxx_std_11)
endif()

# target_compile_definitions(${TPH_PDS_TARGET_NAME}
#   INTERFACE
#     JSON_USE_IMPLICIT_CONVERSIONS=$<BOOL:${JSON_ImplicitConversions}>
#     JSON_DIAGNOSTICS=$<BOOL:${JSON_Diagnostics}>
# )

target_include_directories(${TPH_PDS_TARGET_NAME}
  ${TPH_PDS_SYSTEM_INCLUDE} INTERFACE
  $<BUILD_INTERFACE:${TPH_PDS_INCLUDE_BUILD_DIR}>
  $<INSTALL_INTERFACE:include>
)

# Install a pkg-config file, so other tools can find this.
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pkg-config.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
)

##
## TESTS
## Create and configure the unit test target.
##
if (TPH_BuildTests)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

##
## EXAMPLES
## Create and configure the example targets.
##
if (TPH_BuildExamples)
  add_subdirectory(examples)
endif()

##
## INSTALL
## Install header files, generate and install cmake config files for find_package().
##
include(CMakePackageConfigHelpers)
# Use a custom package version config file instead of
# write_basic_package_version_file to ensure that it's architecture-independent.
configure_file(
  "cmake/tph_poisson_disk_samplingConfigVersion.cmake.in"
  ${TPH_PDS_CMAKE_VERSION_CONFIG_FILE}
  @ONLY
)
configure_file(
  ${TPH_PDS_CMAKE_CONFIG_TEMPLATE}
  ${TPH_PDS_CMAKE_PROJECT_CONFIG_FILE}
  @ONLY
)

if(TPH_Install)
  install(
    DIRECTORY ${TPH_PDS_INCLUDE_BUILD_DIR}
    DESTINATION ${TPH_PDS_INCLUDE_INSTALL_DIR}
  )
  install(
    FILES ${TPH_PDS_CMAKE_PROJECT_CONFIG_FILE} ${TPH_PDS_CMAKE_VERSION_CONFIG_FILE}
    DESTINATION ${TPH_PDS_CONFIG_INSTALL_DIR}
  )
  export(
    TARGETS ${TPH_PDS_TARGET_NAME}
    NAMESPACE ${PROJECT_NAME}::
    FILE ${TPH_PDS_CMAKE_PROJECT_TARGETS_FILE}
  )
  install(
    TARGETS ${TPH_PDS_TARGET_NAME}
    EXPORT ${TPH_PDS_TARGETS_EXPORT_NAME}
    INCLUDES DESTINATION ${TPH_PDS_INCLUDE_INSTALL_DIR}
  )
  install(
    EXPORT ${TPH_PDS_TARGETS_EXPORT_NAME}
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${TPH_PDS_CONFIG_INSTALL_DIR}
  )
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
    DESTINATION ${TPH_PDS_PKGCONFIG_INSTALL_DIR}
  )
endif()







# cmake_minimum_required(VERSION 3.5)
# project(poisson_disk_sampling CXX)

# message(STATUS "CMake version: ${CMAKE_VERSION}")

# # if command can use IN_LIST
# cmake_policy(SET CMP0057 NEW)

# # For cmake > 3.15 don't add warning levels by default.
# # Probably a little early to enable this for now.
# # cmake_policy(SET CMP0092 NEW)

# list(APPEND CMAKE_MODULE_PATH
#   ${CMAKE_CURRENT_LIST_DIR}/CMake
# )

# include(ThinksHelpers)

# # Include current path
# list(APPEND THINKS_COMMON_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

# option(THINKS_CLANG_TIDY "If ON, clang-tidy is used." OFF)

# if (${THINKS_CLANG_TIDY})
#   find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-9)
#   if (NOT CLANG_TIDY)
#     message(FATAL_ERROR "clang-tidy not found")
#   else()
#     message(STATUS "clang-tidy = ${CLANG_TIDY}")
#     set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY})
#   endif()
# endif()

# # Find dependencies
# ## pthread
# find_package(Threads REQUIRED)

# option(THINKS_RUN_TESTS "If ON, tests will be run." OFF)

# if(${THINKS_RUN_TESTS})
#   # Enable CTest. This will set BUILD_TESTING to ON unless otherwise specified
#   # on the command line.
#   include(CTest)
#   enable_testing()
#   message(STATUS "BUILD_TESTING = ${BUILD_TESTING}")
# endif()

# # Build tests and examples only when NOT included as subproject.
# if (BUILD_TESTING)
#   # JSON
#   set(JSON_BuildTests OFF CACHE INTERNAL "")
#   set(JSON_Install OFF CACHE INTERNAL "")
#   add_subdirectory(external/json)
#   check_target(nlohmann_json::nlohmann_json)
 
#   # Catch
#   add_subdirectory(external/Catch2)
#   check_target(Catch2::Catch2)

#   # Simple-FFT
#   add_library(simple_fft INTERFACE)
#   target_include_directories(simple_fft
#     INTERFACE
#       external/Simple-FFT/include)
#   add_library(d1vanov::simple_fft ALIAS simple_fft)

#   # Hedley
#   add_library(hedley INTERFACE)
#   target_include_directories(hedley
#     INTERFACE
#       external/hedley)
#   add_library(nemequ::hedley ALIAS hedley)

#   # STB
#   add_library(stb INTERFACE)
#   target_include_directories(stb
#     INTERFACE
#       external/stb)
#   add_library(nothings::stb ALIAS stb)
# endif()

# add_subdirectory(thinks)
