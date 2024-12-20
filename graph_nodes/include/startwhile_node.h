#pragma once
#include "basic_node.h"

class StartwhileNode : public BasicNode {
public:
  explicit StartwhileNode();
  virtual ~StartwhileNode();
  bool isContinueReturn = true;
  bool isDoWhile = false;

  std::string getPrintableName();
};