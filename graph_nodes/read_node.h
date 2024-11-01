#ifndef READ_NODE_H
#define READ_NODE_H
#include "graph_node.h"

class ReadNode : public GraphNode {
public:
  explicit ReadNode();
  virtual ~ReadNode();
};
#endif