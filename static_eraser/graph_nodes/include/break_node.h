#pragma once
#include "basic_node.h"

class BreakNode : public BasicNode {
public:
  explicit BreakNode();
  virtual ~BreakNode();

  std::string getPrintableName();
};