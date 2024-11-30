#include "start_node.h"
#include "node_types.h"

StartNode::StartNode() : BasicNode::BasicNode(NodeType::START) {}
StartNode::~StartNode() = default;

// TODO - ADD FUNCTION NAME HERE MAYBE???
std::string StartNode::getPrintableName() { return "Function"; }