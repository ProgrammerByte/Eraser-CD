#pragma once
#include "basic_node.h"

class LockNode : public BasicNode {
public:
  explicit LockNode(std::string varName);
  virtual ~LockNode();

  std::string getPrintableName();
  std::string varName;
};