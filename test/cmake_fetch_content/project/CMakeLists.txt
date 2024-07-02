cmake_minimum_required(VERSION 3.11...3.14)

project(DummyImport C)

include(FetchContent)

get_filename_component(GIT_REPOSITORY_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../.. ABSOLUTE)
FetchContent_Declare(poisson GIT_REPOSITORY ${GIT_REPOSITORY_DIRECTORY} GIT_TAG HEAD)

FetchContent_GetProperties(poisson)
if(NOT poisson_POPULATED)
  FetchContent_Populate(poisson)
  add_subdirectory(${poisson_SOURCE_DIR} ${poisson_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Only set the cxx_standard if it is not set by someone else.
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(poisson_test main.c)
target_link_libraries(poisson_test
PRIVATE 
  thinks::tph_poisson
)
