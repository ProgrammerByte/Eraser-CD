#pragma once
#include "basic_node.h"

class StartwhileNode : public BasicNode {
public:
  explicit StartwhileNode();
  virtual ~StartwhileNode();
  GraphNode *continueReturn = this;
  bool isDoWhile = false;

  std::string getPrintableName();
};