#include "eraser_ignore_off_node.h"
#include "node_types.h"

EraserIgnoreOffNode::EraserIgnoreOffNode()
    : BasicNode::BasicNode(NodeType::ERASER_IGNORE_OFF) {}
EraserIgnoreOffNode::~EraserIgnoreOffNode() = default;

std::string EraserIgnoreOffNode::getPrintableName() {
  return "Eraser ignore off";
}