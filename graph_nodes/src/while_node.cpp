#include "while_node.h"
#include "node_types.h"

WhileNode::WhileNode() : GraphNode::GraphNode(NodeType::WHILE) {}
WhileNode::~WhileNode() = default;

void WhileNode::add(GraphNode *node) { whileNode = node; }

void WhileNode::add(EndwhileNode *node) { endWhile = node; }