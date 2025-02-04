#include "continue_return_node.h"
#include "node_types.h"

ContinueReturnNode::ContinueReturnNode()
    : BasicNode::BasicNode(NodeType::CONTINUE_RETURN) {}
ContinueReturnNode::~ContinueReturnNode() = default;

std::string ContinueReturnNode::getPrintableName() { return "continue return"; }