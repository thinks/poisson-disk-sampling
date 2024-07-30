# Default to c++20.
if(DEFINED CMAKE_CXX_STANDARD)
  set(_cxx_standard ${CMAKE_CXX_STANDARD})
else()
  set(_cxx_standard 20)
endif()

# Default to c11.
if(DEFINED CMAKE_C_STANDARD)
  set(_c_standard ${CMAKE_C_STANDARD})
else()
  set(_c_standard 11)
endif()

if(MSVC)
  set(_c_warnings
    /W4 # Baseline reasonable warnings
    /w14242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
    /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
    /w14263 # 'function': member function does not override any base class virtual member function
    /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not
            # be destructed correctly
    /w14287 # 'operator': unsigned/negative constant mismatch
    /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside
            # the for-loop scope
    /w14296 # 'operator': expression is always 'boolean_value'
    /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
    /w14545 # expression before comma evaluates to a function which is missing an argument list
    /w14546 # function call before comma missing argument list
    /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
    /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
    /w14555 # expression has no effect; expected expression with side- effect
    /w14619 # pragma warning: there is no warning number 'number'
    /w14640 # Enable warning on thread un-safe static member initialization
    /w14826 # Conversion from 'type1' to 'type2' is sign-extended. This may cause unexpected runtime behavior.
    /w14905 # wide string literal cast to 'LPSTR'
    /w14906 # string literal cast to 'LPWSTR'
    /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
    /permissive- # standards conformance mode for MSVC compiler.
  )
  if(TPH_POISSON_WARNINGS_AS_ERRORS)
    message(TRACE "Warnings are treated as errors")
    list(APPEND _c_warnings /WX)
  endif()
elseif(CMAKE_C_COMPILER_ID MATCHES ".*Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(_c_warnings
    -Wall
    -Wextra # reasonable and standard
    -Wshadow # warn the user if a variable declaration shadows one from a parent context
    # catch hard to track down memory errors
    -Wcast-align # warn for potential performance problem casts
    -Wunused # warn on anything being unused
    -Wpedantic # warn if non-standard C++ is used
    -Wconversion # warn on type conversions that may lose data
    -Wsign-conversion # warn on sign conversions
    -Wnull-dereference # warn if a null dereference is detected
    -Wdouble-promotion # warn if float is implicit promoted to double
    -Wformat=2 # warn on security issues around functions that format output (ie printf)
    -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
  )
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(_c_warnings
      ${_c_warnings}
      -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
      -Wduplicated-cond # warn if if / else chain has duplicated conditions
      -Wduplicated-branches # warn if if / else branches have duplicated code
      -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
    )
  endif()
  if(TPH_POISSON_WARNINGS_AS_ERRORS)
    message(TRACE "Warnings are treated as errors")
    list(APPEND _c_warnings -Werror)
  endif()
else()
  message(AUTHOR_WARNING "No compiler warnings set for C compiler: '${CMAKE_C_COMPILER_ID}'")
endif()

add_library(tph_poisson_f STATIC
  "tph_poisson_f.c")
target_link_libraries(tph_poisson_f PUBLIC thinks::tph_poisson)
target_compile_options(tph_poisson_f PRIVATE ${_c_warnings})
set_target_properties(tph_poisson_f PROPERTIES
  C_STANDARD ${_c_standard}
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)
add_library(thinks::tph_poisson_f ALIAS tph_poisson_f)

add_executable(tph_poisson_test 
  "test.cpp")
target_link_libraries(tph_poisson_test PRIVATE thinks::tph_poisson_f)
set_target_properties(tph_poisson_test PROPERTIES
  CXX_STANDARD ${_cxx_standard}
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
add_test(NAME tph_poisson_test COMMAND tph_poisson_test)

add_executable(tph_poisson_vec_test 
  "tph_poisson_vec_test.c")
target_link_libraries(tph_poisson_vec_test PRIVATE thinks::tph_poisson)
if(NOT MSVC)
  target_link_libraries(tph_poisson_vec_test PRIVATE m)
endif()
set_target_properties(tph_poisson_vec_test PROPERTIES
  C_STANDARD ${_c_standard}
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
)
add_test(NAME tph_poisson_vec_test COMMAND tph_poisson_vec_test)

unset(_cxx_standard)
unset(_c_standard)
unset(_c_warnings)

#
# add_executable(tphvec_test 
#   "tphvec.h" 
#   "tphvec.c" 
#   "tphvec_test.c")
# # target_link_libraries(tph_poisson_test PRIVATE thinks::tph_poisson)
# set_target_properties(tph_poisson_test PROPERTIES
#   C_STANDARD ${_c_standard}
#   C_STANDARD_REQUIRED ON
#   C_EXTENSIONS OFF
# )
# add_test(NAME tphvec_test COMMAND tphvec_test)
