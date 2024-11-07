#pragma once
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