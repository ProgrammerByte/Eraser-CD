#include "graph_node.h"
#include "node_types.h"

GraphNode::GraphNode(NodeType type) : type(type) {}

GraphNode::~GraphNode() = default;

void GraphNode::add(GraphNode *node) { next = node; }