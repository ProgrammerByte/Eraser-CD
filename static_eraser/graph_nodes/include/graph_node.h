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
  bool eraserIgnoreOn = true;

  explicit GraphNode(NodeType type);
  virtual ~GraphNode() = default;

  virtual void add(GraphNode *node) = 0;
  virtual GraphNode *getNextNode() = 0;
  virtual std::string getPrintableName() = 0;

  std::string getPrintableNameWithId();

  int id = 0;
  virtual std::vector<GraphNode *> getNextNodes() = 0;
  std::string getNodeType();
};

struct CompareGraphNode {
  bool operator()(const GraphNode *a, const GraphNode *b) {
    return a->id > b->id;
  }
};