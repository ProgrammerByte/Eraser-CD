#pragma once
#include "break_node.h"
#include "call_graph.h"
#include "continue_node.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "eraser_sets.h"
#include "function_call_node.h"
#include "function_eraser_sets.h"
#include "function_variable_locksets.h"
#include "if_node.h"
#include "lock_node.h"
#include "read_node.h"
#include "return_node.h"
#include "set_operations.h"
#include "start_node.h"
#include "thread_create_node.h"
#include "thread_join_node.h"
#include "unlock_node.h"
#include "variable_locks.h"
#include "while_node.h"
#include "write_node.h"

class VariableLocksets {
public:
  explicit VariableLocksets(CallGraph *callGraph,
                            FunctionVariableLocksets *functionVariableLocksets);
  virtual ~VariableLocksets() = default;

  void updateLocksets(std::unordered_map<std::string, StartNode *> funcCfgs,
                      std::vector<std::string> changedFunctions);

private:
  CallGraph *callGraph;
  FunctionVariableLocksets *functionVariableLocksets;

  std::priority_queue<GraphNode *, std::vector<GraphNode *>, CompareGraphNode>
      forwardQueue;

  std::vector<GraphNode *> backwardQueue;

  std::unordered_map<std::string, StartNode *> funcCfgs;
  std::string currFunc;
  std::string currTest;
  VariableLocks variableLocksets;
  std::unordered_map<std::string, std::set<std::string>> funcCallLocksets;
  std::unordered_map<GraphNode *, std::set<std::string>> nodeLocks;

  bool variableRead(std::string varName, std::set<std::string> &locks);
  bool variableWrite(std::string varName, std::set<std::string> &locks);

  bool handleNode(FunctionCallNode *node, std::set<std::string> &locks);
  bool handleNode(ThreadCreateNode *node, std::set<std::string> &locks);
  bool handleNode(ThreadJoinNode *node, std::set<std::string> &locks);
  bool handleNode(LockNode *node, std::set<std::string> &locks);
  bool handleNode(UnlockNode *node, std::set<std::string> &locks);
  bool handleNode(ReadNode *node, std::set<std::string> &locks);
  bool handleNode(WriteNode *node, std::set<std::string> &locks);
  bool handleNode(GraphNode *node, std::set<std::string> &locks);

  void addNodeToQueue(GraphNode *startNode, GraphNode *nextNode);
  void handleFunction(GraphNode *startNode, std::set<std::string> &startLocks);
};