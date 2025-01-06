#include "graph_visualizer.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

template <typename T> std::string GraphVisualizer::pointerToString(T *ptr) {
  if (ptr == nullptr) {
    return "nullptr";
  }

  std::size_t hashValue = std::hash<T *>{}(ptr);

  const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
  const size_t alphabetSize = sizeof(alphabet) - 1;

  std::string result;
  while (hashValue > 0) {
    result += alphabet[hashValue % alphabetSize];
    hashValue /= alphabetSize;
  }

  return result;
}

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
    visitNode(nextNode);
    visitNode(node);
  }
}

void GraphVisualizer::visualizeGraph(StartNode *node) {
  visitNode(node);

  std::ofstream file("graph.dot");
  if (!file.is_open()) {
    std::cerr << "Error: Unable to create DOT file!" << std::endl;
    return;
  }

  // create a pointer to node map.

  file << "digraph G {\n";
  for (GraphNode *node : nodes) {
    file << "  " << pointerToString(node) << " [label=\"" << nodeNames[node]
         << "\"];\n";
  }
  for (GraphNode *start : nodes) {
    for (GraphNode *end : adjacencyMatrix[start]) {
      file << "  " << pointerToString(start) << " -> " << pointerToString(end)
           << ";\n";
    }
  }
  file << "}\n";

  file.close();
  system("dot -Tpng graph.dot -o graph.png");
}