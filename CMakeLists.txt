cmake_minimum_required(VERSION 3.0.0)

project(piper_example VERSION 0.1 LANGUAGES CXX)

set(CLANGFORMAT_EXECUTABLE clang-format-12)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(ClangFormat)

set(PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config")

find_package(Threads)

if (NOT DEBUG)
    #set (COMPILE_FLAGS "-O3 -Wall -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3 -flto -stdlib=libstdc++ -fsanitize=address")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set (COMPILE_FLAGS "-O3 -Wall -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3 -flto -stdlib=libstdc++")
    else()
        set (COMPILE_FLAGS "-O3 -Wall -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3 -flto")
    endif()
else()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set (COMPILE_FLAGS "-O0 -g -fno-inline -Wall  -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3")
    else()
        set (COMPILE_FLAGS "-O0 -g -fno-inline -Wall  -funsigned-char -fno-omit-frame-pointer -pedantic -mssse3")
    endif()
endif()

add_definitions(${COMPILE_FLAGS})

file(GLOB_RECURSE LIB_SRC "example.cpp" "src/**" "include/**")
file(GLOB_RECURSE EXAMPLE_SRC "example.cpp" "src/**" "include/**")
file(GLOB_RECURSE EXAMPLE2_SRC "example2.cpp" "src/**" "include/**")
file(GLOB_RECURSE EXAMPLE3_SRC "example3.cpp" "src/**" "include/**")
file(GLOB_RECURSE EXAMPLE4_SRC "example4.cpp" "src/**" "include/**")

include_directories("include")

add_executable(example ${EXAMPLE_SRC})
add_executable(example2 ${EXAMPLE2_SRC})
add_executable(example3 ${EXAMPLE3_SRC})
add_executable(example4 ${EXAMPLE4_SRC}) 

target_link_libraries(example ${CMAKE_THREAD_LIBS_INIT})
#target_link_libraries(example /usr/lib/clang/10.0.1/lib/linux/libclang_rt.asan-x86_64.a)
#target_link_libraries(example -ldl)
target_link_libraries(example2 ${CMAKE_THREAD_LIBS_INIT})
#target_link_libraries(example2 /usr/lib/clang/10.0.1/lib/linux/libclang_rt.asan-x86_64.a)
#target_link_libraries(example2 -ldl)
target_link_libraries(example3 ${CMAKE_THREAD_LIBS_INIT})
#target_link_libraries(example3 /usr/lib/clang/10.0.1/lib/linux/libclang_rt.asan-x86_64.a)
#target_link_libraries(example3 -ldl)
target_link_libraries(example4 ${CMAKE_THREAD_LIBS_INIT})
#target_link_libraries(example4 /usr/lib/clang/10.0.1/lib/linux/libclang_rt.asan-x86_64.a)
#target_link_libraries(example4 -ldl)

clangformat_setup(${EXAMPLE_SRC} ${EXAMPLE2_SRC} ${EXAMPLE3_SRC} ${EXAMPLE4_SRC})

add_library(piper STATIC ${LIB_SRC})

