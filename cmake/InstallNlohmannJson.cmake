# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

##
## nlohmann::json
##

option(ENABLE_NLOHMANN_JSON_BUILD "Enable reconfiguring and rerunning the nlohmann::json build" ON)

FetchContent_Declare(
  nlohmann_json_fc
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.7.0
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)
FetchContent_GetProperties(nlohmann_json_fc)
if(NOT nlohmann_json_fc_POPULATED)
  FetchContent_Populate(nlohmann_json_fc)

  if (ENABLE_NLOHMANN_JSON_BUILD)
    set(nlohmann_json_fc_CACHE_ARGS
      "-DCMAKE_INSTALL_PREFIX:STRING=${nlohmann_json_fc_BINARY_DIR}/install" 
      "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
      "-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON"         
      "-DJSON_BuildTests:BOOL=OFF"
      "-DJSON_SystemInclude:BOOL=ON"
    )
    if(CMAKE_TOOLCHAIN_FILE)
      list(APPEND nlohmann_json_fc_CACHE_ARGS 
        "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}"
      )
    else()
      list(APPEND nlohmann_json_fc_CACHE_ARGS 
        "-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}"
        "-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}"
      )
    endif()    

    get_property(isMulti GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(NOT isMulti)
      list(APPEND nlohmann_json_fc_CACHE_ARGS "-DCMAKE_BUILD_TYPE:STRING=Release")
    endif()

    if(CMAKE_GENERATOR_PLATFORM)
      list(APPEND nlohmann_json_fc_GENERATOR_ARGS
        --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
      )
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
      list(APPEND nlohmann_json_fc_GENERATOR_ARGS
        --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
      )
    endif()

    message(STATUS "Configuring and building nlohmann::json immediately...")
    execute_process(
      COMMAND ${CMAKE_CTEST_COMMAND}
              --build-and-test  ${nlohmann_json_fc_SOURCE_DIR} ${nlohmann_json_fc_BINARY_DIR}
              --build-generator ${CMAKE_GENERATOR} ${nlohmann_json_fc_GENERATOR_ARGS}
              --build-target    install
              --build-noclean
              --build-options   ${nlohmann_json_fc_CACHE_ARGS}
      WORKING_DIRECTORY ${nlohmann_json_fc_SOURCE_DIR}
      OUTPUT_FILE       ${nlohmann_json_fc_BINARY_DIR}/build_output.log
      ERROR_FILE        ${nlohmann_json_fc_BINARY_DIR}/build_output.log
      RESULT_VARIABLE   result
    )
    message(STATUS "nlohmann::json build complete!")
    if(result)
      message(FATAL_ERROR "Failed nlohmann::json build, see build log at:\n"
        "    ${nlohmann_json_fc_BINARY_DIR}/build_output.log")
    endif()
  endif()
endif()

# Confirm that we can find nlohmann::json.
find_package(nlohmann_json 
  #QUIET 
  REQUIRED 
  CONFIG
  PATHS "${nlohmann_json_fc_BINARY_DIR}/install"
  NO_DEFAULT_PATH 
)
if (NOT nlohmann_json_FOUND) 
  message(FATAL_ERROR "nlohmann::json package not found!")
endif()
