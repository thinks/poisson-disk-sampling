# Only set the cxx_standard if it is not set by someone else.
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

# strongly encouraged to enable this globally to avoid conflicts between
# -Wpedantic being enabled and -std=c++20 and -std=gnu++20 for example
# when compiling with PCH enabled
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(poisson_disk_sampling_test "")
target_sources(poisson_disk_sampling_test PRIVATE test.cpp)
target_include_directories(poisson_disk_sampling_test
PUBLIC 
  ${THINKS_COMMON_INCLUDE_DIRS}
)
target_compile_definitions(poisson_disk_sampling_test
PUBLIC 
  ${THINKS_CC_TEST_DEFINES}
)
target_compile_options(poisson_disk_sampling_test
PRIVATE 
  ${THINKS_CC_TEST_COPTS}
)
target_link_libraries(poisson_disk_sampling_test
PRIVATE 
  thinks::poisson_disk_sampling
)

# Add all Thinks targets to a folder in the IDE for organization.
set_property(TARGET ${_NAME} PROPERTY FOLDER ${THINKS_IDE_FOLDER}/test)

set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD ${THINKS_CXX_STANDARD})
set_property(TARGET ${_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

add_test(NAME poisson_disk_sampling_test COMMAND poisson_disk_sampling_test)