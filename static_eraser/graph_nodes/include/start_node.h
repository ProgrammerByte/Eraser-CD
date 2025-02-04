#pragma once
#include "basic_node.h"

class StartNode : public BasicNode {
public:
  explicit StartNode(std::string funcName);
  virtual ~StartNode();

  std::string getPrintableName();

private:
  std::string funcName;
};