#pragma once
#include "basic_node.h"

class EraserIgnoreOnNode : public BasicNode {
public:
  explicit EraserIgnoreOnNode();
  virtual ~EraserIgnoreOnNode();

  std::string getPrintableName();
};