#include "basic_node.h"

BasicNode::BasicNode(NodeType type) : GraphNode::GraphNode(type) {}

BasicNode::~BasicNode() = default;

void BasicNode::add(GraphNode *node) { next = node; }