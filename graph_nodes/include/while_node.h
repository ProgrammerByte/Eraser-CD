#pragma once
#include "endwhile_node.h"
#include "graph_node.h"
#include <vector>

class WhileNode : public GraphNode {
public:
  GraphNode *whileNode = nullptr;
  EndwhileNode *endWhile = nullptr;
  bool isDoWhile = false;
  explicit WhileNode();
  virtual ~WhileNode();
  GraphNode *getNextNode();
  void add(GraphNode *node);
  void add(EndwhileNode *node);
  std::string getPrintableName();

  std::vector<GraphNode *> getForwardBranches();
  std::vector<GraphNode *> getBackwardBranches();
};