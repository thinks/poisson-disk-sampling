include(FetchContent)

function(fetch_rpmalloc)
  set(options)
  set(oneValueArgs VERSION)
  set(multiValueArgs)
  cmake_parse_arguments(args
      "${options}"
      "${oneValueArgs}"
      "${multiValueArgs}"
      ${ARGN}
  )

  # Fetch a fork of https://github.com/mjansson/rpmalloc that adds a CMake build system.
  FetchContent_Declare(rpmalloc
    GIT_REPOSITORY https://github.com/cognitiv/rpmalloc.git
    GIT_TAG        ${args_VERSION}
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
  )
  FetchContent_GetProperties(rpmalloc)
  if(NOT rpmalloc_POPULATED)
    FetchContent_Populate(rpmalloc)

    set(rpmalloc_CACHE_ARGS
        "-DCMAKE_INSTALL_PREFIX:STRING=${rpmalloc_BINARY_DIR}/install" 
        "-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON"     
        "-DCMAKE_BUILD_TYPE:STRING=Release"     
        "-DBUILD_TESTS:BOOL=OFF"
    )

    set(rpmalloc_GENERATOR_ARGS "")
    if(CMAKE_GENERATOR_PLATFORM)
      list(APPEND rpmalloc_GENERATOR_ARGS
          --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
      )
    endif()
  
    message(STATUS "Configuring and building rpmalloc-${args_VERSION} immediately")
    execute_process(
      COMMAND ${CMAKE_CTEST_COMMAND}
              --build-and-test  ${rpmalloc_SOURCE_DIR} ${rpmalloc_BINARY_DIR}
              --build-generator ${CMAKE_GENERATOR} ${rpmalloc_GENERATOR_ARGS}
              --build-target    install
              --build-noclean
              --build-options   ${rpmalloc_CACHE_ARGS}
      WORKING_DIRECTORY ${rpmalloc_SOURCE_DIR}
      OUTPUT_FILE       ${rpmalloc_BINARY_DIR}/build_output.log
      ERROR_FILE        ${rpmalloc_BINARY_DIR}/build_output.log
      RESULT_VARIABLE   result
    )
    if(result)
      file(READ ${rpmalloc_BINARY_DIR}/build_output.log build_log)
      message(FATAL_ERROR "Result = ${result}\nFailed rpmalloc-${args_VERSION} build, see build log:\n"
        "${build_log}")
      unset(build_log)
    endif()
    unset(result)
    message(STATUS "rpmalloc-${args_VERSION} build complete")
  endif()

  # Confirm that we can find nlohmann_json.
  find_package(rpmalloc 
    QUIET 
    REQUIRED 
    CONFIG
    PATHS "${rpmalloc_BINARY_DIR}/install"
    NO_DEFAULT_PATH 
  )
  if(NOT rpmalloc_FOUND) 
    message(FATAL_ERROR "rpmalloc-${args_VERSION} not found")
  endif()
endfunction()
