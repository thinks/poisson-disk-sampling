cmake_minimum_required(VERSION 3.5)
message(STATUS "CMake version: ${CMAKE_VERSION}")

# Only set the cxx_standard if it is not set by someone else.
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
set(CMAKE_CXX_EXTENSIONS OFF)



# Set the project name and language
project(
  tph_poisson_disk_sampling
  VERSION 0.0.1
  DESCRIPTION ""
  # HOMEPAGE_URL "%%myurl%%"
  LANGUAGES CXX C)

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
if (BUILD_TESTING)
  # JSON
  set(JSON_BuildTests OFF CACHE INTERNAL "")
  set(JSON_Install OFF CACHE INTERNAL "")
  add_subdirectory(external/json)
  check_target(nlohmann_json::nlohmann_json)
 
  # Catch
  add_subdirectory(external/Catch2)
  check_target(Catch2::Catch2)

  # Simple-FFT
  add_library(simple_fft INTERFACE)
  target_include_directories(simple_fft
    INTERFACE
      external/Simple-FFT/include)
  add_library(d1vanov::simple_fft ALIAS simple_fft)

  # Hedley
  add_library(hedley INTERFACE)
  target_include_directories(hedley
    INTERFACE
      external/hedley)
  add_library(nemequ::hedley ALIAS hedley)

  # STB
  add_library(stb INTERFACE)
  target_include_directories(stb
    INTERFACE
      external/stb)
  add_library(nothings::stb ALIAS stb)
endif()

add_subdirectory(thinks)
