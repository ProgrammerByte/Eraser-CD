#pragma once
#include "basic_node.h"

class EraserIgnoreOffNode : public BasicNode {
public:
  explicit EraserIgnoreOffNode();
  virtual ~EraserIgnoreOffNode();

  std::string getPrintableName();
};