include(FetchContent)

function(FetchNlohmannJson)
  set(options)

  set(oneValueArgs
    VERSION
  )

  set(multiValueArgs
  )

  cmake_parse_arguments(args
      "${options}"
      "${oneValueArgs}"
      "${multiValueArgs}"
      ${ARGN}
  )

  FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v${args_VERSION}
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
  )

  FetchContent_GetProperties(nlohmann_json)
  if(NOT nlohmann_json_POPULATED)
    FetchContent_Populate(nlohmann_json)
  
    set(nlohmann_json_CACHE_ARGS
      "-DCMAKE_INSTALL_PREFIX:STRING=${nlohmann_json_BINARY_DIR}/install" 
      "-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON"         
      "-DJSON_BuildTests:BOOL=OFF"
    )
  
    message(STATUS "Configuring and building nlohmann::json-${args_VERSION} immediately")
    execute_process(
      COMMAND ${CMAKE_CTEST_COMMAND}
              --build-and-test  ${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR}
              --build-generator ${CMAKE_GENERATOR} ${nlohmann_json_GENERATOR_ARGS}
              --build-target    install
              --build-noclean
              --build-options   ${nlohmann_json_CACHE_ARGS}
      WORKING_DIRECTORY ${nlohmann_json_SOURCE_DIR}
      OUTPUT_FILE       ${nlohmann_json_BINARY_DIR}/build_output.log
      ERROR_FILE        ${nlohmann_json_BINARY_DIR}/build_output.log
      RESULT_VARIABLE   result
    )
    if(result)
      file(READ ${nlohmann_json_BINARY_DIR}/build_output.log build_log)
      message(FATAL_ERROR "Result = ${result}\nFailed nlohmann::json-${args_VERSION} build, see build log:\n"
        "${build_log}")
      unset(build_log)
    endif()
    unset(result)
    message(STATUS "nlohmann::json-${args_VERSION} build complete")
  endif()
  
  # Confirm that we can find nlohmann_json.
  find_package(nlohmann_json 
    QUIET 
    REQUIRED 
    CONFIG
    PATHS "${nlohmann_json_BINARY_DIR}/install"
    NO_DEFAULT_PATH 
  )
  if (NOT nlohmann_json_FOUND) 
    message(FATAL_ERROR "nlohmann::json-${args_VERSION} not found")
  endif()
endfunction()

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

FetchNlohmannJson(
  VERSION "3.7.0"
)
add_executable(tph_poisson_json "json.cpp")
target_link_libraries(tph_poisson_json PRIVATE 
  nlohmann_json::nlohmann_json 
  thinks::tph_poisson
)
set_target_properties(tph_poisson_json PROPERTIES
  CXX_STANDARD ${_cxx_standard}
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

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

add_executable(tph_poisson_simple_cpp simple.cpp)
target_link_libraries(tph_poisson_simple_cpp PRIVATE thinks::tph_poisson)
set_target_properties(tph_poisson_simple_cpp PROPERTIES
  CXX_STANDARD ${_cxx_standard}
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

add_executable(tph_poisson_simple_c "simple.c")
target_link_libraries(tph_poisson_simple_c PRIVATE thinks::tph_poisson)
if(NOT MSVC)
  target_link_libraries(tph_poisson_simple_c PRIVATE m)
endif()
set_target_properties(tph_poisson_simple_c PROPERTIES
  C_STANDARD ${_c_standard}
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)

unset(_cxx_standard)
unset(_c_standard)
