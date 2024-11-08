# ---- Fetch and install FFTW ----

cmake_policy(PUSH)

if(POLICY CMP0169)
  # Allow calling FetchContent_Populate directly.
  cmake_policy(SET CMP0169 OLD)
endif()
if(POLICY CMP0135)
  # Set the timestamps of extracted contents to the time of extraction.
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

function(fetch_fftw)
  set(options)
  set(oneValueArgs VERSION)
  set(multiValueArgs)
  cmake_parse_arguments(args
      "${options}"
      "${oneValueArgs}"
      "${multiValueArgs}"
      ${ARGN}
  )

  FetchContent_Declare(fftw
    URL "http://fftw.org/fftw-${args_VERSION}.tar.gz"
  )

  FetchContent_GetProperties(fftw)
  if(NOT fftw_POPULATED)
    FetchContent_Populate(fftw)
  
    set(fftw_CACHE_ARGS         
      "-DCMAKE_INSTALL_PREFIX=${fftw_BINARY_DIR}/install"
      "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON" # PIC
  
      "-DBUILD_SHARED_LIBS:BOOL=OFF" # static libs
      "-DBUILD_TESTS:BOOL=OFF" # no tests
      
      "-DENABLE_THREADS:BOOL=ON" # Use pthread
      "-DWITH_COMBINED_THREADS:BOOL=ON" # Don't need to link in fftw3f_threads
  
      "-DENABLE_FLOAT:BOOL=ON" # <float>
  
      # Use SSE, but not AVX.
      "-DENABLE_SSE:BOOL=ON" 
      "-DENABLE_SSE2:BOOL=ON"
      "-DENABLE_AVX:BOOL=OFF"
      "-DENABLE_AVX2:BOOL=OFF"      
  
      "-DDISABLE_FORTRAN:BOOL=ON"
    )
  
    if(CMAKE_TOOLCHAIN_FILE)
      list(APPEND fftw_CACHE_ARGS 
        "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}"
      )
    else()
      list(APPEND fftw_CACHE_ARGS 
        "-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}"
      )
    endif()    
  
    get_property(isMulti GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(NOT isMulti)
      list(APPEND fftw_CACHE_ARGS "-DCMAKE_BUILD_TYPE:STRING=Release")
    endif()
    unset(isMulti)
  
    set(fftw_GENERATOR_ARGS "")
    if(CMAKE_GENERATOR_PLATFORM)
      list(APPEND fftw_GENERATOR_ARGS
        --build-generator-platform "${CMAKE_GENERATOR_PLATFORM}"
      )
    endif()
    if(CMAKE_GENERATOR_TOOLSET)
      list(APPEND fftw_GENERATOR_ARGS
        --build-generator-toolset "${CMAKE_GENERATOR_TOOLSET}"
      )
    endif()
  
    message(STATUS "Configuring and building FFTW-${args_VERSION} immediately")
    execute_process(
      COMMAND ${CMAKE_CTEST_COMMAND}
              --build-and-test  ${fftw_SOURCE_DIR} ${fftw_BINARY_DIR}
              --build-generator ${CMAKE_GENERATOR} ${fftw_GENERATOR_ARGS}
              --build-target    install
              --build-noclean
              --build-options   ${fftw_CACHE_ARGS}
      WORKING_DIRECTORY ${fftw_SOURCE_DIR}
      OUTPUT_FILE       ${fftw_BINARY_DIR}/build_output.log
      ERROR_FILE        ${fftw_BINARY_DIR}/build_output.log
      RESULT_VARIABLE   result
    )
    if(result)
      file(READ ${fftw_BINARY_DIR}/build_output.log build_log)
      message(FATAL_ERROR "Result = ${result}\nFailed FFTW-${args_VERSION} build, see build log:\n"
        "${build_log}")
      unset(build_log)
    endif()
    unset(result)
    message(STATUS "FFTW-${args_VERSION} build complete")
  endif() # fftw_POPULATED
  
  # Confirm that we can find FFTW.
  set(_cmake_import_check_xcframework_for_FFTW3::fftw3f "")
  find_package(FFTW3f
    QUIET
    REQUIRED 
    CONFIG
    PATHS "${fftw_BINARY_DIR}/install"
    NO_DEFAULT_PATH 
  )
  unset(_cmake_import_check_xcframework_for_FFTW3::fftw3f)
  if(NOT FFTW3f_FOUND) 
    message(FATAL_ERROR "FFTW-${args_VERSION} not found")
  endif()
endfunction()

cmake_policy(POP)
