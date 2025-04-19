#include "start_node.h"
#include "node_types.h"

StartNode::StartNode(std::string funcName)
    : funcName(funcName), BasicNode::BasicNode(NodeType::START) {
  eraserIgnoreOn = false;
}
StartNode::~StartNode() = default;

// TODO - ADD FUNCTION NAME HERE MAYBE???
std::string StartNode::getPrintableName() { return "Function " + funcName; }