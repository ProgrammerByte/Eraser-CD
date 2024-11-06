#ifndef CONSTRUCTION_ENVIRONMENT_H
#define CONSTRUCTION_ENVIRONMENT_H
#include "break_node.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "if_node.h"
#include "node_types.h"
#include "return_node.h"
#include "while_node.h"
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