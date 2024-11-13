cmake_minimum_required(VERSION 3.14)
message(STATUS "CMake version: ${CMAKE_VERSION}")

include(cmake/prelude.cmake)
include(cmake/version.cmake)

project(tph_poisson
  VERSION "${tph_poisson_MAJOR_VERSION}.${tph_poisson_MINOR_VERSION}.${tph_poisson_PATCH_VERSION}"
  DESCRIPTION "A single file implementation of Poisson disk sampling in arbitrary dimensions."
  HOMEPAGE_URL "https://github.com/thinks/poisson-disk-sampling"
  LANGUAGES C CXX
)

message(STATUS "${PROJECT_NAME}-${PROJECT_VERSION}")

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(thinks_tph_poisson INTERFACE)
add_library(thinks::tph_poisson ALIAS thinks_tph_poisson)

set_property(
  TARGET thinks_tph_poisson PROPERTY
  EXPORT_NAME tph_poisson
)

target_include_directories(thinks_tph_poisson ${warning_guard}
  INTERFACE
  "\$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${tph_poisson_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(examples)
  endif()
endif()

# ---- Developer mode ----

if(NOT tph_poisson_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of tph_poisson"
  )
endif()

include(cmake/dev-mode.cmake)
