#pragma once
#include "graph_node.h"

class BasicNode : public GraphNode {
public:
  GraphNode *next = nullptr;

  explicit BasicNode(NodeType type);
  virtual ~BasicNode();

  GraphNode *getNextNode();
  void add(GraphNode *node);

  std::vector<GraphNode *> getForwardBranches();
  std::vector<GraphNode *> getBackwardBranches();
};