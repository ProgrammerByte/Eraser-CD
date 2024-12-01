#include "unlock_node.h"
#include "node_types.h"

UnlockNode::UnlockNode(std::string varName)
    : varName(varName), BasicNode::BasicNode(NodeType::UNLOCK) {}
UnlockNode::~UnlockNode() = default;

// TODO - IDENTIFIER
std::string UnlockNode::getPrintableName() { return "Unlock " + varName; }