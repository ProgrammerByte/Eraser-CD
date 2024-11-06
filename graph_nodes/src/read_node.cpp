#include "read_node.h"
#include "node_types.h"

ReadNode::ReadNode() : BasicNode::BasicNode(NodeType::READ) {}
ReadNode::~ReadNode() = default;