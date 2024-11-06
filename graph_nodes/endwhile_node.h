#ifndef ENDWHILE_NODE_H
#define ENDWHILE_NODE_H
#include "basic_node.h"
#include "endwhile_node.h"

class EndwhileNode : public BasicNode {
public:
  int numBreaks = 0;
  explicit EndwhileNode();
  virtual ~EndwhileNode();
};
#endif