#include "break_node.h"
#include "node_types.h"

BreakNode::BreakNode() : BasicNode::BasicNode(NodeType::BREAK) {}
BreakNode::~BreakNode() = default;

std::string BreakNode::getPrintableName() { return "break"; }