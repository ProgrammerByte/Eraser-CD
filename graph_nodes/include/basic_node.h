#ifndef BASIC_NODE_H
#define BASIC_NODE_H
#include "graph_node.h"

class BasicNode : public GraphNode {
public:
  GraphNode *next;

  explicit BasicNode(NodeType type);
  virtual ~BasicNode();

  void add(GraphNode *node);
};
#endif