#include "read_node.h"
#include "node_types.h"

ReadNode::ReadNode(std::string varName)
    : varName(varName), BasicNode::BasicNode(NodeType::READ) {}
ReadNode::~ReadNode() = default;

// TODO - IDENTIFIER
std::string ReadNode::getPrintableName() { return "Read " + varName; }