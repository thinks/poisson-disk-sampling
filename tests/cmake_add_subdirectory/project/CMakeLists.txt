cmake_minimum_required(VERSION 3.1)

project(DummyImport CXX)

set(TPH_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(${tph_pds_source}
   ${CMAKE_CURRENT_BINARY_DIR}/tph_pds)

add_executable(with_namespace_target main.cpp)
target_link_libraries(with_namespace_target tph_poisson_disk_sampling::tph_poisson_disk_sampling)

add_executable(without_namespace_target main.cpp)
target_link_libraries(without_namespace_target tph_poisson_disk_sampling)

# if(NOT MSVC)
#     add_executable(without_exceptions main.cpp)
#     target_link_libraries(without_exceptions nlohmann_json::nlohmann_json)
#     target_compile_definitions(without_exceptions PRIVATE JSON_NOEXCEPTION)
#     target_compile_options(without_exceptions PRIVATE -fno-exceptions)
# endif()