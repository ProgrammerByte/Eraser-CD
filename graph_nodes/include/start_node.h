#pragma once
#include "basic_node.h"

class StartNode : public BasicNode {
public:
  explicit StartNode();
  virtual ~StartNode();

  std::string getPrintableName();
};