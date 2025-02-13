#include "eraser_ignore_on_node.h"
#include "node_types.h"

EraserIgnoreOnNode::EraserIgnoreOnNode()
    : BasicNode::BasicNode(NodeType::ERASER_IGNORE_ON) {}
EraserIgnoreOnNode::~EraserIgnoreOnNode() = default;

std::string EraserIgnoreOnNode::getPrintableName() {
  return "Eraser ignore on";
}