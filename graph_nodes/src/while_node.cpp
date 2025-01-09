#include "while_node.h"
#include "node_types.h"

WhileNode::WhileNode() : GraphNode::GraphNode(NodeType::WHILE) {}
WhileNode::~WhileNode() = default;

GraphNode *WhileNode::getNextNode() {
  if (visits == 0) {
    return whileNode;
  } else if (visits == 1) {
    return endWhile;
  } else {
    return nullptr;
  }
}

void WhileNode::add(GraphNode *node) { whileNode = node; }

void WhileNode::add(EndwhileNode *node) { endWhile = node; }

std::string WhileNode::getPrintableName() { return "while"; }

std::vector<GraphNode *> WhileNode::getNextNodes() {
  std::vector<GraphNode *> result = {};
  if (whileNode != nullptr) {
    result.push_back(whileNode);
  }
  if (endWhile != nullptr) {
    result.push_back(endWhile);
  }
  return result;
}