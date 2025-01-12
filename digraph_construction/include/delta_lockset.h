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
#include "thread_join_node.h"
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
  bool operator()(const GraphNode *a, const GraphNode *b) {
    return a->id > b->id;
  }
};

struct EraserSets {
  std::set<std::string> locks;
  std::set<std::string> unlocks;
};

inline bool operator==(const EraserSets &lhs, const EraserSets &rhs) {
  return lhs.locks == rhs.locks && lhs.unlocks == rhs.unlocks;
}

inline bool operator!=(const EraserSets &lhs, const EraserSets &rhs) {
  return !(lhs == rhs);
}

class DeltaLockset {
public:
  explicit DeltaLockset(CallGraph *callGraph);
  virtual ~DeltaLockset() = default;

  void updateLocksets(std::unordered_map<std::string, StartNode *> funcCfgs,
                      std::vector<std::string> changedFunctions);

private:
  bool recursive;
  CallGraph *callGraph;

  std::priority_queue<GraphNode *, std::vector<GraphNode *>, CompareGraphNode>
      forwardQueue;

  std::vector<GraphNode *> backwardQueue;

  std::unordered_map<std::string, StartNode *> funcCfgs;
  std::string currFunc;
  std::unordered_map<std::string, EraserSets> functionSets;
  std::unordered_map<GraphNode *, EraserSets> nodeSets;

  bool handleNode(FunctionCallNode *node, EraserSets &sets);
  bool handleNode(ThreadCreateNode *node, EraserSets &sets);
  bool handleNode(ThreadJoinNode *node, EraserSets &sets);
  bool handleNode(LockNode *node, EraserSets &sets);
  bool handleNode(UnlockNode *node, EraserSets &sets);
  bool handleNode(ReadNode *node, EraserSets &sets);
  bool handleNode(WriteNode *node, EraserSets &sets);
  bool handleNode(ReturnNode *node, EraserSets &sets);
  bool handleNode(GraphNode *node, EraserSets &sets);

  void addNodeToQueue(GraphNode *startNode, GraphNode *nextNode);
  void handleFunction(GraphNode *startNode);
};