#pragma once
#include "node_types.h"
#include <string>

class GraphNode {
public:
  NodeType type;
  int visits = 0;

  explicit GraphNode(NodeType type);
  virtual ~GraphNode() = default;

  virtual void add(GraphNode *node) = 0;
  virtual GraphNode *getNextNode() = 0;
  virtual std::string getPrintableName() = 0;
  std::string getNodeType();
};