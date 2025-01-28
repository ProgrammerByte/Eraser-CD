#include "cumulative_locksets.h"

CumulativeLocksets::CumulativeLocksets(
    CallGraph *callGraph,
    FunctionCumulativeLocksets *functionCumulativeLocksets) {
  this->callGraph = callGraph;
  this->functionCumulativeLocksets = functionCumulativeLocksets;
}

void CumulativeLocksets::updateLocksets() {
  std::vector<std::string> functions =
      functionCumulativeLocksets->getFunctionsForTesting();
  std::vector<std::string> ordering =
      callGraph->deltaLocksetOrdering(functions);

  for (const std::string &funcName : ordering) {
    if (!functionCumulativeLocksets->shouldVisitNode(funcName)) {
      continue;
    }
    functionCumulativeLocksets->updateFunctionCumulativeLocksets(funcName);
  }
}