#include "function_call_node.h"
#include "node_types.h"

// Maybe handle both call and return here???
FunctionCallNode::FunctionCallNode()
    : BasicNode::BasicNode(NodeType::FUNCTION_CALL) {}
FunctionCallNode::~FunctionCallNode() = default;