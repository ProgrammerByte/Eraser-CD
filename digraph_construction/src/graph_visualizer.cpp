#include "graph_visualizer.h"
#include <fstream>
#include <iostream>

void GraphVisualizer::visitNode(GraphNode *node) {
  if (node == nullptr) {
    return;
  }
  if (adjacencyMatrix.find(node) == adjacencyMatrix.end()) {
    nodes.push_back(node);
    adjacencyMatrix.insert({node, std::vector<GraphNode *>(0)});
    nodeNames.insert({node, node->getPrintableName()});
  }
  GraphNode *nextNode = node->getNextNode();
  node->visits += 1;
  if (nextNode != nullptr) {
    adjacencyMatrix[node].push_back(nextNode);
  }
  visitNode(nextNode);
  visitNode(node);
}

void GraphVisualizer::visualizeGraph(StartNode *node) {
  visitNode(node);

  std::ofstream file("graph.dot");
  if (!file.is_open()) {
    std::cerr << "Error: Unable to create DOT file!" << std::endl;
    return;
  }

  file << "digraph G {\n";
  for (GraphNode *node : nodes) {
    file << "  " << node << " [label=\"" << nodeNames[node] << "\"];\n";
  }
  for (GraphNode *start : nodes) {
    for (GraphNode *end : adjacencyMatrix[start]) {
      file << "  " << nodeNames[start] << " -> " << nodeNames[end] << ";\n";
    }
  }
  file << "}\n";

  file.close();
  system("dot -Tpng graph.dot -o graph.png");
}