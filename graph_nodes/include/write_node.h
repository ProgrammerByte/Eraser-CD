#pragma once
#include "basic_node.h"

class WriteNode : public BasicNode {
public:
  explicit WriteNode();
  virtual ~WriteNode();

  std::string getPrintableName();
};