#include "write_node.h"
#include "node_types.h"

WriteNode::WriteNode(std::string varName)
    : varName(varName), BasicNode::BasicNode(NodeType::WRITE) {}
WriteNode::~WriteNode() = default;

std::string WriteNode::getPrintableName() { return "Write " + varName; }