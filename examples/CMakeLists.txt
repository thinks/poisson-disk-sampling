cmake_minimum_required(VERSION 3.14)

project(tph_poissonExamples C CXX)

function(add_example)
  set(options)
  set(oneValueArgs "NAME;SRC")
  set(multiValueArgs DEPS)
  cmake_parse_arguments(args
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${ARGN}
  )

  get_filename_component(_last_ext "${args_SRC}" LAST_EXT)

  add_executable("${args_NAME}" "${args_SRC}")

  if("${_last_ext}" STREQUAL ".cpp")
    target_compile_features("${args_NAME}" PRIVATE cxx_std_17)
  elseif("${_last_ext}" STREQUAL ".c")
    target_compile_features("${args_NAME}" PRIVATE c_std_11)
  endif()
  unset(_last_ext)

  # All examples depend on the main target.  
  target_link_libraries("${args_NAME}" PRIVATE thinks::tph_poisson ${args_DEPS})
  if(NOT MSVC)
    target_link_libraries("${args_NAME}" PRIVATE m)
  endif()

  add_custom_target("run_${args_NAME}" COMMAND "${args_NAME}" VERBATIM)
  add_dependencies("run_${args_NAME}" "${args_NAME}")
  add_dependencies(run-examples "run_${args_NAME}")
endfunction()

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(tph_poisson REQUIRED)
endif()

add_custom_target(run-examples)

# Install dependencies in the build folder.
include(../cmake/fetch-nlohmann_json.cmake)
fetch_nlohmann_json(VERSION "3.11.3")

# Add example targets.
add_example(
  NAME custom_alloc 
  SRC "src/custom_alloc.c"
)
add_example(
  NAME custom_libc 
  SRC "src/custom_libc.c"
)
add_example(
  NAME f64 
  SRC "src/f64.c"
)
add_example(
  NAME json 
  SRC "src/json.cpp"
  DEPS nlohmann_json::nlohmann_json
)
add_example(
  NAME simple_c 
  SRC "src/simple.c"
)
add_example(
  NAME simple_cpp 
  SRC "src/simple.cpp"
)

if (LINUX)
  # FFTW is tricky to build on Mac/Windows. Only build this example on Linux.
  include(../cmake/fetch-fftw.cmake)
  fetch_fftw(VERSION "3.3.10")
  include(../cmake/fetch-stb.cmake)
  fetch_stb()

  add_example(
    NAME periodogram
    SRC "src/periodogram.c"
    DEPS FFTW3::fftw3 nothings::stb)
endif()

# ---- End-of-file commands ----

add_folders(Examples)
