#pragma once
#include "basic_node.h"

class ThreadCreateNode : public BasicNode {
public:
  explicit ThreadCreateNode(std::string functionName, std::string varName,
                            bool global);
  virtual ~ThreadCreateNode();
  std::string getPrintableName();
  std::string functionName;
  std::string varName;
  bool global;
};