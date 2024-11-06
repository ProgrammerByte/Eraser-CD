#ifndef CONSTRUCTION_ENVIRONMENT_H
#define CONSTRUCTION_ENVIRONMENT_H
#include "../graph_nodes/break_node.h"
#include "../graph_nodes/endif_node.h"
#include "../graph_nodes/endwhile_node.h"
#include "../graph_nodes/if_node.h"
#include "../graph_nodes/node_types.h"
#include "../graph_nodes/return_node.h"
#include "../graph_nodes/while_node.h"
#include <memory>
#include <vector>

class ConstructionEnvironment {
public:
  NodeType type;

  explicit ConstructionEnvironment();
  virtual ~ConstructionEnvironment();

  void onAdd(GraphNode *node);
  void onAdd(IfNode *node);
  void onElseAdd();
  void onAdd(EndifNode *node);
  void onAdd(WhileNode *node);
  void onAdd(EndwhileNode *node);
  void onAdd(BreakNode *node);
  void onAdd(ReturnNode *node);

private:
  void callOnAdd(GraphNode *node);

  GraphNode *currNode;
  std::vector<IfNode *> ifStack;
  std::vector<std::vector<BasicNode *>> endifListStack;
  std::vector<WhileNode *> whileStack;
  std::vector<std::vector<BreakNode *>> breakListStack;
  std::vector<GraphNode *> scopeStack;
};
#endif