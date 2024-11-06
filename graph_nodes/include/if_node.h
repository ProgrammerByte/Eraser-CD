#ifndef IF_NODE_H
#define IF_NODE_H
#include "graph_node.h"

class IfNode : public GraphNode {
public:
  GraphNode *ifNode = nullptr;
  GraphNode *elseNode = nullptr;
  bool hasElse = false;
  explicit IfNode();
  virtual ~IfNode();
  void add(GraphNode *node);
};
#endif