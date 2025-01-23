#include "call_graph.h"
#include "construction_environment.h"
#include "cumulative_locksets.h"
#include "database.h"
#include "delta_lockset.h"
#include "function_cumulative_locksets.h"
#include "function_variable_locksets.h"
#include "graph_visualizer.h"
#include "parser.h"
#include "variable_locksets.h"
#include <clang-c/Index.h>
#include <iostream>
#include <unordered_map>
#include <vector>

int main() {
  Database *db = new Database();
  FunctionEraserSets *functionEraserSets = new FunctionEraserSets(db);
  CallGraph *callGraph = new CallGraph(db);
  Parser *parser = new Parser(callGraph);
  parser->parseFile("test_files/single_files/largest_check.c");

  std::vector<std::string> functions = parser->getFunctions();
  std::unordered_map<std::string, StartNode *> funcCfgs = parser->getFuncCfgs();

  GraphVisualizer *visualizer = new GraphVisualizer();
  visualizer->visualizeGraph(funcCfgs[functions[1]]);

  DeltaLockset *deltaLockset = new DeltaLockset(callGraph, functionEraserSets);
  deltaLockset->updateLocksets(funcCfgs, functions);

  FunctionVariableLocksets *functionVariableLocksets =
      new FunctionVariableLocksets(db);

  FunctionCumulativeLocksets *functionCumulativeLocksets =
      new FunctionCumulativeLocksets(db, functionVariableLocksets);

  VariableLocksets *variableLocksets =
      new VariableLocksets(callGraph, functionVariableLocksets);

  CumulativeLocksets *cumulativeLocksets =
      new CumulativeLocksets(callGraph, functionCumulativeLocksets);

  // TODO - INCLUDE PARENTS OF CHANGED DELTA LOCKSETS HERE!!!
  variableLocksets->updateLocksets(funcCfgs, functions);
  // TODO - SELECT WHERE function_variable_locksets.recently_changed = 1
  cumulativeLocksets->updateLocksets(funcCfgs, functions);

  functionEraserSets->markFunctionEraserSetsAsOld();
  functionVariableLocksets->markFunctionVariableLocksetsAsOld();

  // staticEraser->testFunction(funcCfgs, "main");
  std::set<std::string> dataRaces =
      functionCumulativeLocksets->detectDataRaces();

  std::cout << "Variables with data races:" << std::endl;
  for (const std::string &dataRace : dataRaces) {
    std::cout << dataRace << std::endl;
  }
}