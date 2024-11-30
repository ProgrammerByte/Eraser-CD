#include "unlock_node.h"
#include "node_types.h"

UnlockNode::UnlockNode() : BasicNode::BasicNode(NodeType::UNLOCK) {}
UnlockNode::~UnlockNode() = default;

// TODO - IDENTIFIER
std::string UnlockNode::getPrintableName() { return "Unlock"; }