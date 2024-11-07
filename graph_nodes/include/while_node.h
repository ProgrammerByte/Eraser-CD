#pragma once
#include "endwhile_node.h"
#include "graph_node.h"

class WhileNode : public GraphNode {
public:
  GraphNode *whileNode;
  EndwhileNode *endWhile;
  explicit WhileNode();
  virtual ~WhileNode();
  void add(GraphNode *node);
  void add(EndwhileNode *node);
};