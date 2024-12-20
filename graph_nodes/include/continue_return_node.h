#pragma once
#include "basic_node.h"

class ContinueReturnNode : public BasicNode {
public:
  explicit ContinueReturnNode();
  virtual ~ContinueReturnNode();

  std::string getPrintableName();
};