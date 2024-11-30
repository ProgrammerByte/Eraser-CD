#include "if_node.h"
#include "node_types.h"

IfNode::IfNode() : GraphNode::GraphNode(NodeType::IF) {}
IfNode::~IfNode() = default;

void IfNode::add(GraphNode *node) {
  if (hasElse) {
    elseNode = node;
  } else {
    ifNode = node;
  }
}

std::string IfNode::getPrintableName() { return "if"; }