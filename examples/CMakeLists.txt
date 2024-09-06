cmake_minimum_required(VERSION 3.14)

project(tph_poissonExamples C CXX)

function(add_example SRC)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs DEPS)
  cmake_parse_arguments(args
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
  )

  get_filename_component(_name "${SRC}" NAME_WE)
  get_filename_component(_last_ext "${SRC}" LAST_EXT)

  add_executable("${_name}" "${SRC}")

  if("${_last_ext}" STREQUAL ".cpp")
    target_compile_features("${_name}" PRIVATE cxx_std_17)
  elseif("${_last_ext}" STREQUAL ".c")
    target_compile_features("${_name}" PRIVATE c_std_11)
  endif()
  unset(_last_ext)

  # All examples depend on the main target.  
  target_link_libraries("${_name}" PRIVATE thinks::tph_poisson ${args_DEPS})
  if(NOT MSVC)
    target_link_libraries("${_name}" PRIVATE m)
  endif()

  add_custom_target("run_${_name}" COMMAND "${_name}" VERBATIM)
  add_dependencies("run_${_name}" "${_name}")
  add_dependencies(run-examples "run_${_name}")
endfunction()

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(tph_poisson REQUIRED)
endif()

add_custom_target(run-examples)

include(../cmake/fetch-nlohmann_json.cmake)
fetch_nlohmann_json(VERSION "3.7.0")
add_example(json.cpp DEPS nlohmann_json::nlohmann_json)
# add_example(periodogram.c DEPS fftw3f nothings::stb)
add_example(simple_c.c)
add_example(simple_cpp.cpp)

add_folders(Examples)


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
