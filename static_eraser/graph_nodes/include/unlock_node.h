#pragma once
#include "basic_node.h"

class UnlockNode : public BasicNode {
public:
  explicit UnlockNode(std::string varName);
  virtual ~UnlockNode();

  std::string getPrintableName();
  std::string varName;
};