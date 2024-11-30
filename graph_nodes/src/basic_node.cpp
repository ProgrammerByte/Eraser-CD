#include "basic_node.h"

BasicNode::BasicNode(NodeType type) : GraphNode::GraphNode(type) {}

BasicNode::~BasicNode() = default;

GraphNode *BasicNode::getNextNode() {
  if (visits == 0) {
    return next;
  }
  return nullptr;
}

void BasicNode::add(GraphNode *node) { next = node; }