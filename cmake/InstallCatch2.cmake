# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

##
## Catch2
##

option(ENABLE_CATCH2_BUILD "Enable reconfiguring and rerunning the Catch2 build." ON)

FetchContent_Declare(catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        f526ff0fc37ae00ff2c0dc8f6550bf8209c30afc # v3.0.0-preview5
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)

FetchContent_GetProperties(catch2)
if(NOT catch2_POPULATED)
  FetchContent_Populate(catch2)

  if (ENABLE_CATCH2_BUILD)
    set(catch2_CACHE_ARGS
      "-DCMAKE_INSTALL_PREFIX:STRING=${catch2_BINARY_DIR}/install" 
      "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
      "-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON"
      "-DCATCH_ENABLE_WERROR=OFF"       
      "-DCATCH_BUILD_TESTING:BOOL=OFF"
      "-DCATCH_INSTALL_DOCS:BOOL=OFF"   
      "-DCATCH_INSTALL_HELPERS:BOOL=OFF"   
    )
    if(CMAKE_TOOLCHAIN_FILE)
      list(APPEND catch2_CACHE_ARGS 
        "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}"
      )
    else()
      list(APPEND catch2_CACHE_ARGS 
        "-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}"
      )
    endif()    

    get_property(isMulti GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(NOT isMulti)
      list(APPEND catch2_CACHE_ARGS "-DCMAKE_BUILD_TYPE:STRING=Release")
    endif()

    if(CMAKE_GENERATOR_PLATFORM)
      list(APPEND catch2_GENERATOR_ARGS
        --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
      )
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
      list(APPEND catch2_GENERATOR_ARGS
        --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
      )
    endif()

    message(STATUS "Configuring and building Catch2 immediately...")
    execute_process(
      COMMAND ${CMAKE_CTEST_COMMAND}
              --build-and-test  ${catch2_SOURCE_DIR} ${catch2_BINARY_DIR}
              --build-generator ${CMAKE_GENERATOR} ${catch2_GENERATOR_ARGS}
              --build-target    install
              --build-noclean
              --build-options   ${catch2_CACHE_ARGS}
      WORKING_DIRECTORY ${catch2_SOURCE_DIR}
      OUTPUT_FILE       ${catch2_BINARY_DIR}/build_output.log
      ERROR_FILE        ${catch2_BINARY_DIR}/build_output.log
      RESULT_VARIABLE   result
    )
    message(STATUS "Catch2 build complete!")
    if(result)
      message(FATAL_ERROR "Failed Catch2 build, see build log at:\n"
        "    ${catch2_BINARY_DIR}/build_output.log")
    endif()
  endif()
endif()

# Confirm that we can find Catch2.
find_package(Catch2 
  #QUIET 
  REQUIRED 
  CONFIG
  PATHS "${catch2_BINARY_DIR}/install"
  NO_DEFAULT_PATH 
) 
if (NOT Catch2_FOUND) 
  message(FATAL_ERROR "Catch2 not found")
endif()
