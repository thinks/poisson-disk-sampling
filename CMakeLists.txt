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



#include(cmake/PreventInSourceBuilds.cmake)


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


