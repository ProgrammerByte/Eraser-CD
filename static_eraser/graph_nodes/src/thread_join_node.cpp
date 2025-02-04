#include "thread_join_node.h"
#include "node_types.h"

ThreadJoinNode::ThreadJoinNode(std::string varName, bool global)
    : varName(varName),
      global(global), BasicNode::BasicNode(NodeType::THREAD_JOIN) {}

ThreadJoinNode::~ThreadJoinNode() = default;

std::string ThreadJoinNode::getPrintableName() {
  return "pthread_join " + std::string(global ? "global " : "") +
         "var: " + varName;
}