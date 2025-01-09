#pragma once
#include "break_node.h"
#include "call_graph.h"
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
#include <functional>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

struct CompareGraphNode {
  bool operator()(const GraphNode *&a, const GraphNode *&b) {
    return a->id > b->id;
  }
};

class DeltaLockset {
public:
  explicit DeltaLockset(CallGraph *callGraph);
  virtual ~DeltaLockset() = default;

  void updateLocksets(std::unordered_map<std::string, StartNode *> funcCfgs,
                      std::vector<std::string> changedFunctions);

private:
  CallGraph *callGraph;

  std::priority_queue<GraphNode *, std::vector<GraphNode *>, CompareGraphNode>
      forwardQueue;

  std::vector<GraphNode *> backwardQueue;

  std::unordered_map<std::string, StartNode *> funcCfgs;
  std::string currFunc;
  std::unordered_map<std::string, std::set<std::string>>
      deltaLocksets; // TODO - ADD UNLOCKING TOO
  std::unordered_map<GraphNode *, std::set<std::string>> nodeLockSets;

  void handleNode(GraphNode *node, std::set<std::string> &locks);
  void handleNode(FunctionCallNode *node, std::set<std::string> &locks);
  void handleNode(LockNode *node, std::set<std::string> &locks);
  void handleNode(UnlockNode *node, std::set<std::string> &locks);
  void handleNode(ReturnNode *node, std::set<std::string> &locks);

  void addNodeToQueue(GraphNode *startNode, GraphNode *nextNode);
  void handleFunction(GraphNode *startNode);
  void checkNextNodes(GraphNode *node, std::set<std::string> &locks);
  void checkNextNodes(FunctionCallNode *node, std::set<std::string> &locks);
};