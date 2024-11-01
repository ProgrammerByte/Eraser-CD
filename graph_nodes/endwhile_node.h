#ifndef ENDWHILE_NODE_H
#define ENDWHILE_NODE_H
#include "endwhile_node.h"
#include "graph_node.h"

class EndwhileNode : public GraphNode {
public:
  int numBreaks = 0;
  explicit EndwhileNode();
  virtual ~EndwhileNode();
};
#endif