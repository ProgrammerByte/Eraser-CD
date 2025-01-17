#pragma once
#include "call_graph.h"
#include "function_cumulative_locksets.h"
#include "set_operations.h"
#include "start_node.h"
#include <unordered_map>
#include <vector>

class CumulativeLocksets {
public:
  explicit CumulativeLocksets(
      CallGraph *callGraph,
      FunctionCumulativeLocksets *functionCumulativeLocksets);
  virtual ~CumulativeLocksets() = default;

  void updateLocksets(std::unordered_map<std::string, StartNode *> funcCfgs,
                      std::vector<std::string> changedFunctions);

private:
  CallGraph *callGraph;
  FunctionCumulativeLocksets *functionCumulativeLocksets;

  std::unordered_map<std::string, StartNode *> funcCfgs;
};