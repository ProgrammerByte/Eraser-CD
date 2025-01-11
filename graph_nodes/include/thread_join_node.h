#pragma once
#include "basic_node.h"

class ThreadJoinNode : public BasicNode {
public:
  explicit ThreadJoinNode(std::string varName, bool global);
  virtual ~ThreadJoinNode();
  std::string getPrintableName();
  std::string varName;
  bool global;
};