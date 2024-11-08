# ---- Fetch and install STB ----

cmake_policy(PUSH)

if(POLICY CMP0169)
  # Allow calling FetchContent_Populate directly.
  cmake_policy(SET CMP0169 OLD)
endif()

include(FetchContent)

function(fetch_stb)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs)
  cmake_parse_arguments(args
      "${options}"
      "${oneValueArgs}"
      "${multiValueArgs}"
      ${ARGN}
  )

  FetchContent_Declare(nothings_stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        0dd01c26ef47750512cdc1830581480c25a19f1d # master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
  )

  FetchContent_GetProperties(nothings_stb)
  if(NOT nothings_stb_POPULATED)
    FetchContent_Populate(nothings_stb)
  
    message(STATUS "Configuring and building stb immediately")
    message(STATUS "stb build complete")
  endif() # nothings_stb_POPULATED

  add_library(nothings::stb INTERFACE IMPORTED)

  set_target_properties(nothings::stb PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${nothings_stb_SOURCE_DIR}"
  )  

endfunction()

cmake_policy(POP)
