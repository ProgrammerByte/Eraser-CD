#pragma once
#include "node_types.h"
#include <string>
#include <vector>

class GraphNode {
public:
  NodeType type;
  int visits = 0;
  int priorNodes = 1;
  int priorNodesVisited = 0;

  explicit GraphNode(NodeType type);
  virtual ~GraphNode() = default;

  virtual void add(GraphNode *node) = 0;
  virtual GraphNode *getNextNode() = 0;
  virtual std::string getPrintableName() = 0;

  std::string getPrintableNameWithId();

  int id = 0;

  // TODO - I BET ALL THIS STUFF BELOW HERE CAN JUST GO!!!
  virtual std::vector<GraphNode *> getForwardBranches() = 0;
  virtual std::vector<GraphNode *>
  getBackwardBranches() = 0; // TODO - NO NOTION OF BACKWARD BRANCHES NEEDED IN
                             // THIS PROTOTYPE, ONLY NEEDED FOR GOTO

  // TODO - IN FACT MAYBE SPLIT GOTO INTO UPPER AND LOWER S.T UPPER = ONES WE
  // VISIT FROM BEHIND HENCE NEED ALL TO CONTINUE WHEREAS FROM IN FRONT ONLY
  // NEED ONE TO CONTINUE
  std::string getNodeType();
};