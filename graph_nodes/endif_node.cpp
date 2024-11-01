#include "endif_node.h"
#include "node_types.h"

EndifNode::EndifNode() : GraphNode::GraphNode(NodeType::ENDIF) {}
EndifNode::~EndifNode() = default;