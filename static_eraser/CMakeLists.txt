cmake_minimum_required(VERSION 3.10)
project(static_eraser VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# the following lines are for debugging with gdb
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -fno-omit-frame-pointer")
# set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG -fno-omit-frame-pointer")

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

file(GLOB DIGRAPH_SRC CONFIGURE_DEPENDS src/*.cpp database/src/*.cpp graph_nodes/src/*.cpp)
include_directories("/usr/lib/llvm-14/include" ./include ./database/include ./graph_nodes/include)
link_directories("/usr/lib/llvm-14/lib")

find_package(SQLite3 REQUIRED)

add_executable(static_eraser src/main.cpp ${DIGRAPH_SRC})

target_link_libraries(static_eraser PRIVATE clang SQLite::SQLite3)
target_include_directories(static_eraser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)