cmake_minimum_required(VERSION 3.14)

project(tph_poissonExamples C CXX)

function(add_example NAME)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs DEPS)
  cmake_parse_arguments(args
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
  )

  add_executable("${NAME}" "${NAME}")

  get_filename_component(_last_ext "${NAME}" LAST_EXT)
  if("${_last_ext}" STREQUAL ".cpp")
    target_compile_features("${NAME}" PRIVATE cxx_std_20)
  elseif("${_last_ext}" STREQUAL ".c")
    target_compile_features("${NAME}" PRIVATE c_std_11)
  endif()
  unset(_last_ext)

  target_link_libraries("${NAME}" PRIVATE thinks::tph_poisson ${args_DEPS})
  if(NOT MSVC)
    target_link_libraries("${NAME}" PRIVATE m)
  endif()  
  add_custom_target("run_${NAME}" COMMAND "${NAME}" VERBATIM)
  add_dependencies("run_${NAME}" "${NAME}")
  add_dependencies(run-examples "run_${NAME}")
endfunction()

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(tph_poisson REQUIRED)
endif()

add_custom_target(run-examples)

include(../cmake/fetch-nlohmann_json.cmake)
fetch_nlohmann_json(VERSION "3.7.0")
add_example(json.cpp
  DEPS nlohmann_json::nlohmann_json
)
# add_example(periodogram.c
#   DEPS fftw
# )
add_example(simple_c.c)
add_example(simple_cpp.cpp)

add_folders(Examples)

# Default to c++20.
if (DEFINED CMAKE_CXX_STANDARD)
  set(_cxx_standard ${CMAKE_CXX_STANDARD})
else()
  set(_cxx_standard 20)
endif()

# Default to c11.
if (DEFINED CMAKE_C_STANDARD)
  set(_c_standard ${CMAKE_C_STANDARD})
else()
  set(_c_standard 11)
endif()


# FetchFFTW(
#   VERSION "x.y.z"
# )
# FetchStb(
#   VERSION "x.y.z"
# )
# add_executable(tph_poisson_periodogram "periodogram.c")
# target_link_libraries(tph_poisson_periodogram PRIVATE 
#   nothings::stb... 
#   fftw3f...
#   thinks::tph_poisson
# )
# set_target_properties(tph_poisson_periodogram PROPERTIES
#   C_STANDARD ${_c_standard}
#   C_STANDARD_REQUIRED ON
#   C_EXTENSIONS OFF
# )

# add_executable(tph_poisson_simple_cpp simple.cpp)
# target_link_libraries(tph_poisson_simple_cpp PRIVATE thinks::tph_poisson)
# set_target_properties(tph_poisson_simple_cpp PROPERTIES
#   CXX_STANDARD ${_cxx_standard}
#   CXX_STANDARD_REQUIRED ON
#   CXX_EXTENSIONS OFF
# )

# add_executable(tph_poisson_simple_c "simple.c")
# target_link_libraries(tph_poisson_simple_c PRIVATE thinks::tph_poisson)
# if(NOT MSVC)
#   target_link_libraries(tph_poisson_simple_c PRIVATE m)
# endif()
# set_target_properties(tph_poisson_simple_c PROPERTIES
#   C_STANDARD ${_c_standard}
#   C_STANDARD_REQUIRED ON
#   C_EXTENSIONS OFF
# )

unset(_cxx_standard)
unset(_c_standard)
