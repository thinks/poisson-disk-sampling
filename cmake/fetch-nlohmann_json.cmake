include(FetchContent)

function(fetch_nlohmann_json)
  set(options)
  set(oneValueArgs VERSION)
  set(multiValueArgs)
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
  
    set(nlohmann_json_GENERATOR_ARGS "")
    if(CMAKE_GENERATOR_PLATFORM)
      list(APPEND nlohmann_json_GENERATOR_ARGS
          --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
      )
    endif()

    message(STATUS "Configuring and building nlohmann_json-${args_VERSION} immediately")
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
      message(FATAL_ERROR "Result = ${result}\nFailed nlohmann_json-${args_VERSION} build, see build log:\n"
        "${build_log}")
      unset(build_log)
    endif()
    unset(result)
    message(STATUS "nlohmann_json-${args_VERSION} build complete")
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
    message(FATAL_ERROR "nlohmann_json-${args_VERSION} not found")
  endif()
endfunction()
