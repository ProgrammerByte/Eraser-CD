#include "thread_create_node.h"
#include "node_types.h"

ThreadCreateNode::ThreadCreateNode(std::string functionName)
    : functionName(functionName), BasicNode::BasicNode(
                                      NodeType::THREAD_CREATE) {}

ThreadCreateNode::~ThreadCreateNode() = default;

std::string ThreadCreateNode::getPrintableName() {
  return "pthread_create on " + functionName;
}