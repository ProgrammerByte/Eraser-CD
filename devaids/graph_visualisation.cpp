#include <fstream>
#include <iostream>
#include <string>

void createGraph() {
  std::ofstream file("graph.dot");
  if (!file.is_open()) {
    std::cerr << "Error: Unable to create DOT file!" << std::endl;
    return;
  }

  file << "digraph G {\n";
  file << "  A [label=\"Node A\"];\n";
  file << "  B [label=\"Node B\"];\n";
  file << "  C [label=\"Node C\"];\n";
  file << "  A -> B [label=\"A to B\"];\n";
  file << "  B -> C [label=\"B to C\"];\n";
  file << "  C -> A [label=\"C to A\"];\n";
  file << "}\n";

  file.close();
  system("dot -Tpng graph.dot -o graph.png");
}

int main() {
  createGraph();
  return 0;
}