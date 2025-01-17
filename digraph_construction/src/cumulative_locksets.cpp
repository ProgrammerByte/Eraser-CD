#include "cumulative_locksets.h"

CumulativeLocksets::CumulativeLocksets(
    CallGraph *callGraph,
    FunctionCumulativeLocksets *functionCumulativeLocksets) {
  this->callGraph = callGraph;
  this->functionCumulativeLocksets = functionCumulativeLocksets;
}

void CumulativeLocksets::updateLocksets(
    std::unordered_map<std::string, StartNode *> funcCfgs,
    std::vector<std::string> changedFunctions) {
  std::vector<std::string> ordering =
      callGraph->deltaLocksetOrdering(changedFunctions);

  for (const std::string &funcName : ordering) {
    if (!functionCumulativeLocksets->shouldVisitNode(funcName)) {
      continue;
    }
    functionCumulativeLocksets->updateFunctionCumulativeLocksets(funcName);
  }
}