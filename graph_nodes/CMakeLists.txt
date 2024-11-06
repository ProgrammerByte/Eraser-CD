file(GLOB GRAPH_NODES_SRC CONFIGURE_DEPENDS src/*.cpp)
add_library(graph_nodes STATIC ${GRAPH_NODES_SRC})
target_include_directories(graph_nodes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)