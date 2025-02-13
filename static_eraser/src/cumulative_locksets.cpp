#include "cumulative_locksets.h"
#include "debug_tools.h"

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
      debugCout << "CL SKIPPING " << funcName << std::endl;
      continue;
    }
    debugCout << "CL Looking at " << funcName << std::endl;
    functionCumulativeLocksets->updateFunctionCumulativeLocksets(funcName);
  }
}