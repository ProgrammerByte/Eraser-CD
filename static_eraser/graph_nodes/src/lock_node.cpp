#include "lock_node.h"
#include "node_types.h"

LockNode::LockNode(std::string varName)
    : varName(varName), BasicNode::BasicNode(NodeType::LOCK) {}
LockNode::~LockNode() = default;

std::string LockNode::getPrintableName() { return "Lock " + varName; }