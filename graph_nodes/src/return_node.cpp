#include "return_node.h"
#include "node_types.h"

ReturnNode::ReturnNode() : BasicNode::BasicNode(NodeType::RETURN) {}
ReturnNode::~ReturnNode() = default;

std::string ReturnNode::getPrintableName() { return "return"; }