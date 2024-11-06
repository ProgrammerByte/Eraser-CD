#include "endif_node.h"
#include "node_types.h"

EndifNode::EndifNode() : BasicNode::BasicNode(NodeType::ENDIF) {}
EndifNode::~EndifNode() = default;