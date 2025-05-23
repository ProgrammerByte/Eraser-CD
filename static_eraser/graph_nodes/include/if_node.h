#pragma once
#include "graph_node.h"
#include <vector>

class IfNode : public GraphNode {
public:
  GraphNode *ifNode = nullptr;
  GraphNode *elseNode = nullptr;
  bool hasElse = false;
  explicit IfNode();
  virtual ~IfNode();
  GraphNode *getNextNode();
  void add(GraphNode *node);
  std::string getPrintableName();

  std::vector<GraphNode *> getNextNodes();
};