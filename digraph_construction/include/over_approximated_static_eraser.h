#pragma once
#include "break_node.h"
#include "continue_node.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "function_call_node.h"
#include "if_node.h"
#include "lock_node.h"
#include "read_node.h"
#include "return_node.h"
#include "start_node.h"
#include "thread_create_node.h"
#include "unlock_node.h"
#include "while_node.h"
#include "write_node.h"
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class OverApproximatedStaticEraser {
public:
  explicit OverApproximatedStaticEraser() = default;
  virtual ~OverApproximatedStaticEraser() = default;

  void testFunction(std::unordered_map<std::string, StartNode *> funcCfgs,
                    std::string funcName);

private:
  std::vector<std::pair<GraphNode *, std::set<std::string>>> queue;
  std::vector<std::pair<GraphNode *, std::set<std::string>>> barrierQueue;
  std::unordered_map<std::string, StartNode *> funcCfgs;
  std::unordered_map<std::string, std::set<std::string>> lockSets;
  std::unordered_map<GraphNode *, std::set<std::string>> waitingNodeLockSets;

  void handleNode(GraphNode *node, std::set<std::string> &locks);
  void handleNode(ThreadCreateNode *node, std::set<std::string> &locks);
  void handleNode(FunctionCallNode *node, std::set<std::string> &locks);
  void handleNode(LockNode *node, std::set<std::string> &locks);
  void handleNode(UnlockNode *node, std::set<std::string> &locks);
  void handleNode(ReadNode *node, std::set<std::string> &locks);
  void handleNode(WriteNode *node, std::set<std::string> &locks);
  void handleNode(ReturnNode *node, std::set<std::string> &locks);

  void insertNodeIntoQueue(GraphNode *node, std::set<std::string> &locks);
  void checkNextNodes(GraphNode *node, std::set<std::string> &locks);
  void checkNextNodes(ThreadCreateNode *node, std::set<std::string> &locks);
  void checkNextNodes(FunctionCallNode *node, std::set<std::string> &locks);
};