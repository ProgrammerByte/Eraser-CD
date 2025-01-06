#pragma once
#include "basic_node.h"

class WriteNode : public BasicNode {
public:
  explicit WriteNode(std::string varName);
  virtual ~WriteNode();

  std::string getPrintableName();
  std::string varName;
};