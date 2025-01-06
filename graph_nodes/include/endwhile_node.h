#pragma once
#include "basic_node.h"
#include "endwhile_node.h"

class EndwhileNode : public BasicNode {
public:
  explicit EndwhileNode();
  virtual ~EndwhileNode();

  std::string getPrintableName();
};