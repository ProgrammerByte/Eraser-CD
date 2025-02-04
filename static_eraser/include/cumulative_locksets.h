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

  void updateLocksets();

private:
  CallGraph *callGraph;
  FunctionCumulativeLocksets *functionCumulativeLocksets;
};