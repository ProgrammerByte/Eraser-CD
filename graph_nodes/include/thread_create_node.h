#pragma once
#include "basic_node.h"

class ThreadCreateNode : public BasicNode {
public:
  explicit ThreadCreateNode(std::string functionName);
  virtual ~ThreadCreateNode();
  std::string getPrintableName();
  std::string functionName;
};