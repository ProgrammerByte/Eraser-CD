#pragma once
#include "break_node.h"
#include "continue_node.h"
#include "continue_return_node.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "if_node.h"
#include "return_node.h"
#include "start_node.h"
#include "startwhile_node.h"
#include "while_node.h"
#include <memory>
#include <vector>

class ConstructionEnvironment {
public:
  GraphNode *currNode;
  explicit ConstructionEnvironment() = default;
  virtual ~ConstructionEnvironment() = default;

  StartNode *startNewTree(std::string funcName);
  void goBackToStartWhile();
  void onAdd(GraphNode *node);
  void onAdd(IfNode *node);
  void onElseAdd();
  void onAdd(EndifNode *node);
  void onAdd(StartwhileNode *node);
  void onAdd(WhileNode *node);
  void onAdd(EndwhileNode *node);
  void onAdd(BreakNode *node);
  void onAdd(ContinueNode *node);
  void onAdd(ContinueReturnNode *node);
  void onAdd(ReturnNode *node);

private:
  int currId;
  void callOnAdd(GraphNode *node);
  void setNodeId(GraphNode *node);

  std::vector<IfNode *> ifStack;
  std::vector<std::vector<BasicNode *>> endifListStack;
  std::vector<WhileNode *> whileStack;
  std::vector<StartwhileNode *> startwhileStack;
  std::vector<std::vector<BreakNode *>> breakListStack;
  std::vector<std::vector<ContinueNode *>> continueListStack;
};