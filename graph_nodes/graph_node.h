#ifndef GRAPH_NODE_H
#define GRAPH_NODE_H
#include "node_types.h"

class GraphNode {
public:
  NodeType type;

  explicit GraphNode(NodeType type);
  virtual ~GraphNode();

  void add(GraphNode *node);
};
#endif