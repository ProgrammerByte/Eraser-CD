#pragma once
#include "basic_node.h"

class FunctionCallNode : public BasicNode {
public:
  explicit FunctionCallNode(std::string functionName);
  virtual ~FunctionCallNode();
  std::string getPrintableName();
  std::string functionName;
};