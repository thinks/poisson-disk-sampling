cmake_minimum_required(VERSION 3.1)

project(DummyImport CXX)

add_executable(with_private_target main.cpp)
target_include_directories(with_private_target PRIVATE ${tph_pds_source}/include)
set_target_properties(with_private_target PROPERTIES CXX_STANDARD 11)

add_executable(with_private_system_target main.cpp)
target_include_directories(with_private_system_target PRIVATE SYSTEM ${tph_pds_source}/include)
set_target_properties(with_private_system_target PROPERTIES CXX_STANDARD 11)

add_library(Foo STATIC foo.cpp bar.cpp)
target_include_directories(Foo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${tph_pds_source}/include)
set_target_properties(Foo PROPERTIES CXX_STANDARD 11)

add_library(Bar STATIC bar.cpp)
target_link_libraries(Bar PRIVATE Foo)
target_include_directories(Bar PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${tph_pds_source}/include)
set_target_properties(Bar PROPERTIES CXX_STANDARD 11)