
# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
# set(CMAKE_CXX_EXTENSIONS OFF)

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

add_executable(tph_poisson_test test.cpp)
target_link_libraries(tph_poisson_test PRIVATE thinks::tph_poisson)
set_target_properties(tph_poisson_test PROPERTIES
  CXX_STANDARD ${_cxx_standard}
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
# add_test(NAME tph_poisson_test COMMAND tph_poisson_test)

add_executable(tphvec_test 
  "tphvec.h" 
  "tphvec.c" 
  "tphvec_test.c")
# target_link_libraries(tph_poisson_test PRIVATE thinks::tph_poisson)
set_target_properties(tph_poisson_test PROPERTIES
  C_STANDARD ${_c_standard}
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)
add_test(NAME tphvec_test COMMAND tphvec_test)

unset(_cxx_standard)
unset(_c_standard)
