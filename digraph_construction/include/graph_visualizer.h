#pragma once
#include "break_node.h"
#include "continue_node.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "if_node.h"
#include "return_node.h"
#include "start_node.h"
#include "while_node.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class GraphVisualizer {
public:
  explicit GraphVisualizer() = default;
  virtual ~GraphVisualizer() = default;

  void visualizeGraph(StartNode *node);

private:
  std::vector<GraphNode *> nodes;
  std::unordered_map<GraphNode *, std::vector<GraphNode *>> adjacencyMatrix =
      {};
  std::unordered_map<GraphNode *, std::string> nodeNames = {};

  template <typename T> std::string pointerToString(T *ptr);
  void visitNode(GraphNode *node);
};