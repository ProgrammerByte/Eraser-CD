#include "basic_node.h"
#include <vector>

BasicNode::BasicNode(NodeType type) : GraphNode::GraphNode(type) {}

BasicNode::~BasicNode() = default;

GraphNode *BasicNode::getNextNode() {
  if (visits == 0) {
    return next;
  }
  return nullptr;
}

void BasicNode::add(GraphNode *node) { next = node; }

std::vector<GraphNode *> BasicNode::getNextNodes() { return {next}; }