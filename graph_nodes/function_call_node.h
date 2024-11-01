#ifndef FUNCTION_CALL_NODE_H
#define FUNCTION_CALL_NODE_H
#include "graph_node.h"

class FunctionCallNode : public GraphNode {
public:
  explicit FunctionCallNode();
  virtual ~FunctionCallNode();
};
#endif