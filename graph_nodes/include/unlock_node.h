#pragma once
#include "basic_node.h"

class UnlockNode : public BasicNode {
  // TODO - ADD UNLOCK INFO HERE!!!
public:
  explicit UnlockNode(std::string varName);
  virtual ~UnlockNode();

  std::string getPrintableName();

private:
  std::string varName;
};