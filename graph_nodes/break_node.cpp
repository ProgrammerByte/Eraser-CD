#include "break_node.h"
#include "node_types.h"

BreakNode::BreakNode() : GraphNode::GraphNode(NodeType::BREAK) {}
BreakNode::~BreakNode() = default;