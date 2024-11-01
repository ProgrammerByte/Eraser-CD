#include "read_node.h"
#include "node_types.h"

ReadNode::ReadNode() : GraphNode::GraphNode(NodeType::READ) {}
ReadNode::~ReadNode() = default;