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
#include "set_operations.h"
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

// varName -> tid
struct ActiveThreads
    : public std::unordered_map<std::string, std::set<std::string>> {
  ActiveThreads &operator+=(const ActiveThreads &other) {
    for (auto &pair : other) {
      if (find(pair.first) != end()) {
        at(pair.first) += pair.second;
      } else {
        insert(pair);
      }
    }
    return *this;
  }

  ActiveThreads operator+(const ActiveThreads &other) const {
    ActiveThreads result = *this;
    return result += other;
  }

  ActiveThreads &operator-=(const std::set<std::string> &other) {
    for (auto &varName : other) {
      erase(varName);
    }
    return *this;
  }

  ActiveThreads operator-(const std::set<std::string> &other) const {
    ActiveThreads result = *this;
    return result -= other;
  }
};

// Q: (tid -> pending writes)
struct QueuedWrites
    : public std::unordered_map<std::string, std::set<std::string>> {
  std::set<std::string> values() {
    std::set<std::string> result;
    for (auto &pair : *this) {
      result += pair.second;
    }
    return result;
  }

  QueuedWrites &operator+=(const QueuedWrites &other) {
    for (auto &pair : other) {
      if (find(pair.first) != end()) {
        at(pair.first) += pair.second;
      } else {
        insert(pair);
      }
    }
    return *this;
  }

  QueuedWrites operator+(const QueuedWrites &other) const {
    QueuedWrites result = *this;
    return result += other;
  }

  QueuedWrites &operator-=(const std::set<std::string> &other) {
    for (auto &tid : other) {
      erase(tid);
    }
    return *this;
  }

  QueuedWrites operator-(const std::set<std::string> &other) const {
    QueuedWrites result = *this;
    return result -= other;
  }
};

struct VariableLocks
    : public std::unordered_map<std::string, std::set<std::string>> {
  VariableLocks &operator*=(const VariableLocks &other) {
    for (auto &pair : other) {
      if (find(pair.first) != end()) {
        // If the key exists in both sets, intersect the sets
        at(pair.first) *= pair.second;
      } else {
        // If the key only exists in the other set, add it to this set
        insert(pair);
      }
    }
    return *this;
  }

  VariableLocks operator*(const VariableLocks &other) const {
    VariableLocks result = *this;
    return result *= other;
  }

  VariableLocks &operator+=(const VariableLocks &other) {
    for (auto &pair : *this) {
      if (other.find(pair.first) != end()) {
        pair.second += other.at(pair.first);
      } else {
        erase(pair.first);
      }
    }
    return *this;
  }

  VariableLocks operator+(const VariableLocks &other) const {
    VariableLocks result = *this;
    return result += other;
  }
};

struct EraserSets {
  // currently held locks
  std::set<std::string> locks;
  std::set<std::string> unlocks;

  // state machine state representation
  std::set<std::string> externalReads;
  std::set<std::string> internalReads;
  std::set<std::string> externalWrites;
  std::set<std::string> internalWrites;
  std::set<std::string> internalShared;
  std::set<std::string> externalShared;
  std::set<std::string> sharedModified;
  QueuedWrites queuedWrites;

  // threads
  std::set<std::string> finishedThreads;
  ActiveThreads activeThreads;

  bool operator==(const EraserSets &other) const {
    return locks == other.locks && unlocks == other.unlocks &&
           externalReads == other.externalReads &&
           internalReads == other.internalReads &&
           externalWrites == other.externalWrites &&
           internalWrites == other.internalWrites &&
           internalShared == other.internalShared &&
           externalShared == other.externalShared &&
           sharedModified == other.sharedModified &&
           queuedWrites == other.queuedWrites &&
           finishedThreads == other.finishedThreads &&
           activeThreads == other.activeThreads;
  }

  bool operator!=(const EraserSets &other) const { return !(*this == other); }
};

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

  bool recursiveFunctionCall(std::string functionName, EraserSets &sets,
                             bool fromThread);
  void sharedVariableAccessed(std::string varName, EraserSets &sets);

  void threadFinished(std::string varName, EraserSets &sets);

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