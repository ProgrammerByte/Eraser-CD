#pragma once
#include "node_types.h"
#include <string>

class GraphNode {
public:
  NodeType type;

  explicit GraphNode(NodeType type);
  virtual ~GraphNode();

  void add(GraphNode *node){};
  std::string getPrintableName() { return "Stub"; };
};