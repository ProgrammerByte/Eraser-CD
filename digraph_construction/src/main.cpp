#include "call_graph.h"
#include "construction_environment.h"
#include "cumulative_locksets.h"
#include "database.h"
#include "delta_lockset.h"
#include "diff_analysis.h"
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
  DiffAnalysis *diffAnalysis = new DiffAnalysis();
  Parser *parser = new Parser(callGraph);

  std::string repoPath = "~/dissertation/Eraser-CD";
  std::string commitHash1 = "300e894461d8a7cf21a4d2e4b49281e4f940a472";
  std::string commitHash2 = "4127b6c626f3fe9cb311b59c3b14ede9222c420b";
  std::vector<std::string> changedFiles =
      diffAnalysis->getChangedFiles(repoPath, commitHash1, commitHash2);
  std::cout << "Parsing changed files:" << std::endl;
  for (const auto &file : changedFiles) {
    std::cout << file << std::endl;
    parser->parseFile(file.c_str());
  }
  std::cout << std::endl;

  std::vector<std::string> functions = parser->getFunctions();

  GraphVisualizer *visualizer = new GraphVisualizer();
  visualizer->visualizeGraph(funcCfgs[functions[1]]);

  DeltaLockset *deltaLockset =
      new DeltaLockset(callGraph, parser, functionEraserSets);
  deltaLockset->updateLocksets(functions);

  FunctionVariableLocksets *functionVariableLocksets =
      new FunctionVariableLocksets(db);

  FunctionCumulativeLocksets *functionCumulativeLocksets =
      new FunctionCumulativeLocksets(db, functionVariableLocksets);

  VariableLocksets *variableLocksets =
      new VariableLocksets(callGraph, functionVariableLocksets);

  CumulativeLocksets *cumulativeLocksets =
      new CumulativeLocksets(callGraph, functionCumulativeLocksets);

  // TODO - INCLUDE PARENTS OF CHANGED DELTA LOCKSETS HERE!!!
  variableLocksets->updateLocksets(functions);
  // TODO - SELECT WHERE function_variable_locksets.recently_changed = 1
  cumulativeLocksets->updateLocksets(functions);

  functionEraserSets->markFunctionEraserSetsAsOld();
  functionVariableLocksets->markFunctionVariableLocksetsAsOld();
  callGraph->deleteStaleNodes();

  // staticEraser->testFunction("main");
  std::set<std::string> dataRaces =
      functionCumulativeLocksets->detectDataRaces();

  std::cout << "Variables with data races:" << std::endl;
  for (const std::string &dataRace : dataRaces) {
    std::cout << dataRace << std::endl;
  }
}