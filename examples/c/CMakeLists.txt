
add_executable(tph_poisson_c main.c)
target_link_libraries(tph_poisson_c PRIVATE thinks::tph_poisson m)

# Default to c11.
if (DEFINED CMAKE_C_STANDARD)
  set(_c_standard ${CMAKE_C_STANDARD})
else()
  set(_c_standard 11)
endif()

set_target_properties(tph_poisson_c PROPERTIES
  C_STANDARD ${_c_standard}
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)

unset(_c_standard)

# add_test(NAME tph_poisson_c COMMAND tph_poisson_c)