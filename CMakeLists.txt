cmake_minimum_required(VERSION 3.5)
message(STATUS "CMake version: ${CMAKE_VERSION}")

# Set the project name and language
project(
  poisson_disk_sampling
  VERSION 0.0.1
  DESCRIPTION ""
  # HOMEPAGE_URL "%%myurl%%"
  LANGUAGES CXX C)

add_library(poisson_disk_sampling INTERFACE)
target_include_directories(poisson_disk_sampling
  INTERFACE
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
# Project includes for building against.
# target_include_directories(${TARGET_NAME}
#   PRIVATE
#     $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_INCLUDEDIR}>
# )

add_library(thinks::poisson_disk_sampling ALIAS poisson_disk_sampling)

# To gain access to standard install directory variables, such as CMAKE_INSTALL_LIBDIR.
include(GNUInstallDirs)

# Install public headers.
install(
  FILES "include/tph/poisson_disk_sampling.h"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tph
)

# Install the library and export it as a public target.
install(
  TARGETS poisson_disk_sampling
  EXPORT ${CMAKE_PROJECT_NAME}-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# This module is for exporting targets and installing CMake package
# files which are used to import _this_ project into _other_ projects
# in a robust manner.

# Install exported library targets.
install(
    EXPORT ${CMAKE_PROJECT_NAME}-targets
    NAMESPACE thinks::
    FILE ${CMAKE_PROJECT_NAME}Targets.cmake
    DESTINATION cmake
)

# Configure and write <Project>Config.cmake to provide package import entry point.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
    @ONLY
)

# Configure and write  <Project>ConfigVersion.cmake for version compatibility management.
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

# Install the package configuration files.
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)



include(cmake/PreventInSourceBuilds.cmake)

# if command can use IN_LIST
cmake_policy(SET CMP0057 NEW)

# For cmake > 3.15 don't add warning levels by default.
# Probably a little early to enable this for now.
# cmake_policy(SET CMP0092 NEW)

list(APPEND CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_LIST_DIR}/CMake
)

include(ThinksHelpers)

# Include current path
list(APPEND THINKS_COMMON_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

option(THINKS_CLANG_TIDY "If ON, clang-tidy is used." OFF)

if (${THINKS_CLANG_TIDY})
  find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-9)
  if (NOT CLANG_TIDY)
    message(FATAL_ERROR "clang-tidy not found")
  else()
    message(STATUS "clang-tidy = ${CLANG_TIDY}")
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY})
  endif()
endif()

# Find dependencies
## pthread
find_package(Threads REQUIRED)

option(THINKS_RUN_TESTS "If ON, tests will be run." OFF)

if(${THINKS_RUN_TESTS})
  # Enable CTest. This will set BUILD_TESTING to ON unless otherwise specified
  # on the command line.
  include(CTest)
  enable_testing()
  message(STATUS "BUILD_TESTING = ${BUILD_TESTING}")
endif()

# Build tests and examples only when NOT included as subproject.
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

# Adding the tests:
include(CTest)

if(BUILD_TESTING)
  # message(AUTHOR_WARNING "Building Tests. Be sure to check out test/constexpr_tests.cpp for constexpr testing")
  enable_testing()
  add_subdirectory(test)
endif()
