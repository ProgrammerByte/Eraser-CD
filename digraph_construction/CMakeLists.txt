cmake_minimum_required(VERSION 3.10)
project(digraph_construction VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB DIGRAPH_SRC CONFIGURE_DEPENDS src/*.cpp database/src/*.cpp)
include_directories("/usr/lib/llvm-14/include" ./include ./database/include)
link_directories("/usr/lib/llvm-14/lib")

find_package(SQLite3 REQUIRED)

add_executable(digraph_construction src/parser.cpp ${DIGRAPH_SRC})

target_link_libraries(digraph_construction PRIVATE clang graph_nodes SQLite::SQLite3)
target_include_directories(digraph_construction PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)