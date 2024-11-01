#include "function_call_node.h"
#include "node_types.h"

// Maybe handle both call and return here???
FunctionCallNode::FunctionCallNode()
    : GraphNode::GraphNode(NodeType::FUNCTION_CALL) {}
FunctionCallNode::~FunctionCallNode() = default;