cmake_minimum_required(VERSION 3.0.0)

project(piper_example VERSION 0.1 LANGUAGES CXX)

set(CLANGFORMAT_EXECUTABLE clang-format-10)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(ClangFormat)

set(PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config")

find_package(Threads)

if (NOT DEBUG)
    set (COMPILE_FLAGS "-O3 -Wall -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3 -flto -stdlib=libstdc++")
else()
    set (COMPILE_FLAGS "-O0 -g -fno-inline -Wall  -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3")
endif()

add_definitions(${COMPILE_FLAGS})

file(GLOB_RECURSE SOURCES "main.cpp" "src/**" "include/**")

include_directories("include")

add_executable(example ${SOURCES})

target_link_libraries(example ${CMAKE_THREAD_LIBS_INIT})

target_clangformat_setup(example)
