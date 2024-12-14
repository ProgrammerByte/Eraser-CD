#pragma once
#include "basic_node.h"

class StartwhileNode : public BasicNode {
public:
  explicit StartwhileNode();
  virtual ~StartwhileNode();

  std::string getPrintableName();
};