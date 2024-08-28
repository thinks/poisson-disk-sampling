include(cmake/folders.cmake)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()

option(BUILD_MCSS_DOCS "Build documentation using Doxygen and m.css" OFF)
if(BUILD_MCSS_DOCS)
  # include(cmake/docs.cmake)
  # message(FATAL_ERROR "m.css docs not implemented!")
endif()

option(ENABLE_COVERAGE "Enable coverage support separate from CTest's" OFF)
if(ENABLE_COVERAGE)
  # include(cmake/coverage.cmake)
  # message(FATAL_ERROR "coverage not implemented!")
endif()

include(cmake/lint-targets.cmake)
# include(cmake/spell-targets.cmake)

add_folders(Project)
