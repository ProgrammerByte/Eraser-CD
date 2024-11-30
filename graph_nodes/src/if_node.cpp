#include "if_node.h"
#include "node_types.h"

IfNode::IfNode() : GraphNode::GraphNode(NodeType::IF) {}
IfNode::~IfNode() = default;

GraphNode *IfNode::getNextNode() {
  if (visits == 0) {
    return ifNode;
  } else if (visits == 1) {
    return elseNode;
  }
  return nullptr;
}

void IfNode::add(GraphNode *node) {
  if (hasElse) {
    elseNode = node;
  } else {
    ifNode = node;
  }
}

std::string IfNode::getPrintableName() { return "if"; }