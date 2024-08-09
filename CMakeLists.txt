cmake_minimum_required(VERSION 3.12)
message(STATUS "CMake version: ${CMAKE_VERSION}")


##
## Project
##
set(_tph_poisson_hdr "include/thinks/tph_poisson.h")
file(STRINGS
     ${_tph_poisson_hdr}
     _ver_tmp
     REGEX "#define TPH_POISSON_MAJOR_VERSION .*$")
string(REGEX MATCHALL "[0-9]+" TPH_POISSON_MAJOR_VERSION ${_ver_tmp})
file(STRINGS
     ${_tph_poisson_hdr}
     _ver_tmp
     REGEX "#define TPH_POISSON_MINOR_VERSION .*$")
string(REGEX MATCHALL "[0-9]+" TPH_POISSON_MINOR_VERSION ${_ver_tmp})
file(STRINGS
     ${_tph_poisson_hdr}
     _ver_tmp
     REGEX "#define TPH_POISSON_PATCH_VERSION .*$")
string(REGEX MATCHALL "[0-9]+" TPH_POISSON_PATCH_VERSION ${_ver_tmp})
unset(_ver_tmp)
unset(_tph_poisson_hdr)

# Set the project name, version, language, etc.
project(tph_poisson
  VERSION "${TPH_POISSON_MAJOR_VERSION}.${TPH_POISSON_MINOR_VERSION}.${TPH_POISSON_PATCH_VERSION}"
  DESCRIPTION ""
  HOMEPAGE_URL "https://github.com/thinks/poisson-disk-sampling"
  LANGUAGES CXX C)
message(STATUS "${PROJECT_NAME}-${PROJECT_VERSION}")


##
## Target
##
add_library(tph_poisson INTERFACE)
add_library(thinks::tph_poisson ALIAS tph_poisson)
target_include_directories(tph_poisson
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)


##
## Options
##
# Determine if building as a subproject (using add_subdirectory) or 
# if it is the main project.
set(_main_project OFF)
set(_sub_project ON)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(_main_project ON)
    set(_sub_project OFF)
endif()

option(TPH_POISSON_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(TPH_POISSON_BUILD_TESTS        "Enable test build targets when BUILD_TESTING is enabled." ${_main_project})
option(TPH_POISSON_BUILD_EXAMPLES     "Enable example build targets." ${_main_project})

unset(_main_project)
unset(_sub_project)


##
## Examples
##
if(TPH_POISSON_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif() 


##
## Tests
##
if(TPH_POISSON_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()


## 
## Install.
## 
if(CMAKE_SKIP_INSTALL_RULES)
  return()
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs) # Standard install variables, e.g. CMAKE_INSTALL_INCLUDEDIR.

configure_file(
  "cmake/tph_poissonConfigVersion.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  @ONLY
)
configure_file(
  "cmake/config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  @ONLY
)

install(
  DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(
  FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" 
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" 
  DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}"
)
export(
  TARGETS ${PROJECT_NAME}
  NAMESPACE thinks::
  FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
)
install(
  TARGETS ${PROJECT_NAME}
  EXPORT "${PROJECT_NAME}Targets"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(
  EXPORT "${PROJECT_NAME}Targets"
  NAMESPACE thinks::
  DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}"
)


# Install public headers.
# install(
#   FILES "include/tph/poisson.h"
#   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tph
# )

# # Install the library and export it as a public target.
# install(
#   TARGETS tph_poisson
#   EXPORT ${CMAKE_PROJECT_NAME}-targets
#   #LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#   #RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
# )

# # This module is for exporting targets and installing CMake package
# # files which are used to import _this_ project into _other_ projects
# # in a robust manner.

# # Install exported library targets.
# install(
#     EXPORT ${CMAKE_PROJECT_NAME}-targets
#     NAMESPACE thinks::
#     FILE ${CMAKE_PROJECT_NAME}Targets.cmake
#     DESTINATION cmake
# )

# # Configure and write <Project>Config.cmake to provide package import entry point.
# configure_file(
#     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
#     ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
#     @ONLY
# )

# # Configure and write <Project>ConfigVersion.cmake for version compatibility management.
# include(CMakePackageConfigHelpers)
# write_basic_package_version_file(
#     ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
#     VERSION ${CMAKE_PROJECT_VERSION}
#     COMPATIBILITY SameMinorVersion
# )

# # Install the package configuration files.
# install(
#     FILES
#         ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}Config.cmake
#         ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ConfigVersion.cmake
#     DESTINATION ${CMAKE_INSTALL_PREFIX}
# )



#include(cmake/PreventInSourceBuilds.cmake)

# if command can use IN_LIST
# cmake_policy(SET CMP0057 NEW)

# For cmake > 3.15 don't add warning levels by default.
# Probably a little early to enable this for now.
# cmake_policy(SET CMP0092 NEW)

# list(APPEND CMAKE_MODULE_PATH
#   ${CMAKE_CURRENT_LIST_DIR}/CMake
# )

# include(ThinksHelpers)

# Include current path
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

# Find dependencies
## pthread
# find_package(Threads REQUIRED)

# option(THINKS_RUN_TESTS "If ON, tests will be run." OFF)

# if(${THINKS_RUN_TESTS})
#   # Enable CTest. This will set BUILD_TESTING to ON unless otherwise specified
#   # on the command line.
#   include(CTest)
#   enable_testing()
#   message(STATUS "BUILD_TESTING = ${BUILD_TESTING}")
# endif()

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
