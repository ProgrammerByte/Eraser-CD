#include "endwhile_node.h"
#include "node_types.h"

EndwhileNode::EndwhileNode() : BasicNode::BasicNode(NodeType::ENDWHILE) {}
EndwhileNode::~EndwhileNode() = default;

std::string EndwhileNode::getPrintableName() { return "end while"; }