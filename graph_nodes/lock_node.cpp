#include "lock_node.h"
#include "node_types.h"

LockNode::LockNode() : GraphNode::GraphNode(NodeType::LOCK) {}
LockNode::~LockNode() = default;