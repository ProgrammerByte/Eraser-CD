#include "return_node.h"
#include "node_types.h"

ReturnNode::ReturnNode() : GraphNode::GraphNode(NodeType::RETURN) {}
ReturnNode::~ReturnNode() = default;