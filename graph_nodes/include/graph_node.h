#pragma once
#include "node_types.h"
#include <string>

class GraphNode {
public:
  NodeType type;
  int visits = 0;

  explicit GraphNode(NodeType type);
  virtual ~GraphNode();

  void add(GraphNode *node){};
  GraphNode *getNextNode() { return nullptr; }
  std::string getPrintableName() { return "Stub"; };
};