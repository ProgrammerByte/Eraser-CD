#include "continue_node.h"
#include "node_types.h"

ContinueNode::ContinueNode() : GraphNode::GraphNode(NodeType::CONTINUE) {}
ContinueNode::~ContinueNode() = default;