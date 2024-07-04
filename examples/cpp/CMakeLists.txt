
# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
# set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tph_poisson_cpp main.cpp)
target_link_libraries(tph_poisson_cpp PRIVATE thinks::tph_poisson)

# Default to c++20.
if (DEFINED CMAKE_CXX_STANDARD)
  set(_cxx_standard ${CMAKE_CXX_STANDARD})
else()
  set(_cxx_standard 20)
endif()

set_target_properties(tph_poisson_cpp PROPERTIES
  CXX_STANDARD ${_cxx_standard}
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

unset(_cxx_standard)

# add_test(NAME tph_poisson_test COMMAND tph_poisson_test)