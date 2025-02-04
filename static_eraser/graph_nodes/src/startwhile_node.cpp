#include "startwhile_node.h"
#include "node_types.h"

StartwhileNode::StartwhileNode() : BasicNode::BasicNode(NodeType::STARTWHILE) {}
StartwhileNode::~StartwhileNode() = default;

std::string StartwhileNode::getPrintableName() { return "start while"; }