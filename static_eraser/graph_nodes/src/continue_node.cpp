#include "continue_node.h"
#include "node_types.h"

ContinueNode::ContinueNode() : BasicNode::BasicNode(NodeType::CONTINUE) {}
ContinueNode::~ContinueNode() = default;

std::string ContinueNode::getPrintableName() { return "continue"; }