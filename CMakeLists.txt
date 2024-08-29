cmake_minimum_required(VERSION 3.14)
# message(STATUS "CMake version: ${CMAKE_VERSION}")

# Allow calling FetchContent_Populate directly.
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

include(cmake/prelude.cmake)
include(cmake/version.cmake)

project(tph_poisson
  VERSION "${tph_poisson_MAJOR_VERSION}.${tph_poisson_MINOR_VERSION}.${tph_poisson_PATCH_VERSION}"
  DESCRIPTION "A single file implementation of Poisson disk sampling in arbitrary dimensions."
  HOMEPAGE_URL "https://github.com/thinks/poisson-disk-sampling"
  LANGUAGES C CXX)
# message(STATUS "${PROJECT_NAME}-${PROJECT_VERSION}")

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

# ##
# ## Options
# ##
# # Determine if building as a subproject (using add_subdirectory) or 
# # if it is the main project.
# set(_main_project OFF)
# set(_sub_project ON)
# if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
#     set(_main_project ON)
#     set(_sub_project OFF)
# endif()

# option(TPH_POISSON_WARNINGS_AS_ERRORS "Treat warnings as errors." OFF)
# option(TPH_POISSON_USE_ASAN           "Enable sanitizers." OFF)
# option(TPH_POISSON_BUILD_TESTS        "Enable test build targets when BUILD_TESTING is enabled." ${_main_project})
# option(TPH_POISSON_BUILD_EXAMPLES     "Enable example build targets." ${_main_project})

# unset(_main_project)
# unset(_sub_project)


# ##
# ## Examples
# ##
# if(TPH_POISSON_BUILD_EXAMPLES)
#   add_subdirectory(examples)
# endif() 


# ##
# ## Tests
# ##
# if(TPH_POISSON_BUILD_TESTS)
#   include(CTest)
#   enable_testing()
#   add_subdirectory(test)
# endif()


# ## 
# ## Install.
# ## 
# if(CMAKE_SKIP_INSTALL_RULES)
#   return()
# endif()

# include(CMakePackageConfigHelpers)
# include(GNUInstallDirs) # Standard install variables, e.g. CMAKE_INSTALL_INCLUDEDIR.

# configure_file(
#   "cmake/tph_poissonConfigVersion.cmake.in"
#   "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
#   @ONLY
# )
# configure_file(
#   "cmake/config.cmake.in"
#   "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
#   @ONLY
# )

# install(
#   DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
#   DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
# )
# install(
#   FILES 
#     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" 
#     "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" 
#   DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}"
# )
# export(
#   TARGETS ${PROJECT_NAME}
#   NAMESPACE thinks::
#   FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
# )
# install(
#   TARGETS ${PROJECT_NAME}
#   EXPORT "${PROJECT_NAME}Targets"
#   INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
# )
# install(
#   EXPORT "${PROJECT_NAME}Targets"
#   NAMESPACE thinks::
#   DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}"
# )



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


