cmake_minimum_required(VERSION 3.20.0)
project(fy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/$<0:>)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/$<0:>)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/$<0:>)
endforeach()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(fy ${SOURCES})

target_include_directories(fy PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_options(fy PRIVATE ${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs core bitwriter executionengine native mcjit)
target_link_libraries(fy ${llvm_libs})