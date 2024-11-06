#ifndef FUNCTION_CALL_NODE_H
#define FUNCTION_CALL_NODE_H
#include "basic_node.h"

class FunctionCallNode : public BasicNode {
public:
  explicit FunctionCallNode();
  virtual ~FunctionCallNode();
};
#endif