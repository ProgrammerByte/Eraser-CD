#pragma once
#include "basic_node.h"
#include "endwhile_node.h"

class EndwhileNode : public BasicNode {
public:
  int numBreaks = 0;
  explicit EndwhileNode();
  virtual ~EndwhileNode();

  std::string getPrintableName();
};