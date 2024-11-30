#pragma once
#include "basic_node.h"

class ReadNode : public BasicNode {
public:
  explicit ReadNode();
  virtual ~ReadNode();

  std::string getPrintableName();
};