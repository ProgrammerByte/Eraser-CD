#pragma once
#include "basic_node.h"

class ReadNode : public BasicNode {
public:
  explicit ReadNode(std::string varName);
  virtual ~ReadNode();

  std::string getPrintableName();

private:
  std::string varName;
};