#include "start_node.h"
#include "node_types.h"

StartNode::StartNode(std::string funcName)
    : funcName(funcName), BasicNode::BasicNode(NodeType::START) {
  eraserIgnoreOn = false;
}
StartNode::~StartNode() = default;

std::string StartNode::getPrintableName() { return "Function " + funcName; }