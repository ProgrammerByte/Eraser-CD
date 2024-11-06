#include "lock_node.h"
#include "node_types.h"

LockNode::LockNode() : BasicNode::BasicNode(NodeType::LOCK) {}
LockNode::~LockNode() = default;