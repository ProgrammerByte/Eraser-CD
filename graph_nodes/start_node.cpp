#include "start_node.h"
#include "node_types.h"

StartNode::StartNode() : GraphNode::GraphNode(NodeType::START) {}
StartNode::~StartNode() = default;