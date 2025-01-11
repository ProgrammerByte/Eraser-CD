#include "thread_create_node.h"
#include "node_types.h"

ThreadCreateNode::ThreadCreateNode(std::string functionName,
                                   std::string varName, bool global)
    : functionName(functionName), varName(varName),
      global(global), BasicNode::BasicNode(NodeType::THREAD_CREATE) {}

ThreadCreateNode::~ThreadCreateNode() = default;

std::string ThreadCreateNode::getPrintableName() {
  return "pthread_create " + functionName + ", " + (global ? "global " : "") +
         "var: " + varName;
}