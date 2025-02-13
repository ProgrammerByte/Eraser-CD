#include "call_graph.h"
#include "construction_environment.h"
#include "cumulative_locksets.h"
#include "database.h"
#include "debug_tools.h"
#include "delta_lockset.h"
#include "diff_analysis.h"
#include "file_includes.h"
#include "function_cumulative_locksets.h"
#include "function_variable_locksets.h"
#include "graph_visualizer.h"
#include "parser.h"
#include "variable_locksets.h"
#include <chrono>
#include <clang-c/Index.h>
#include <iostream>
#include <sys/resource.h>
#include <unordered_map>
#include <vector>

void logTimeSinceLast(
    std::string label,
    std::chrono::time_point<std::chrono::high_resolution_clock> &currTime) {
  auto nextTime = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(nextTime - currTime)
          .count();
  std::cout << label << duration << "ms" << std::endl;
  currTime = nextTime;
}

int main() {
  auto startTime = std::chrono::high_resolution_clock::now();
  auto currTime = startTime;

  bool initialCommit = false;
  Database *db = new Database(initialCommit);
  FunctionEraserSets *functionEraserSets = new FunctionEraserSets(db);
  CallGraph *callGraph = new CallGraph(db);
  FileIncludes *fileIncludes = new FileIncludes(db);
  DiffAnalysis *diffAnalysis = new DiffAnalysis(fileIncludes);
  Parser *parser = new Parser(callGraph, fileIncludes);

  std::string repoPath = "~/dissertation/Eraser-CD";
  std::set<std::string> changedFiles;
  if (initialCommit) {
    // changedFiles = {"test_files/single_files/largest_check.c"};
    changedFiles = {"test_files/largest_check_multi_file/main.c",
                    "test_files/largest_check_multi_file/recur.c",
                    "test_files/largest_check_multi_file/recur.h",
                    "test_files/largest_check_multi_file/globals.h",
                    "test_files/largest_check_multi_file/largest_check.c",
                    "test_files/largest_check_multi_file/largest_check.h"};
    // changedFiles = {"test_files/single_files/largest_check.c"};
    // changedFiles = {"test_files/test.c"};
    changedFiles = {"test_files/Splash-4/altered/fft.c"};

    std::string repoPath = "test_files/Splash-4/altered/barnes";
    changedFiles = diffAnalysis->getAllFiles(repoPath);
  } else {
    std::string commitHash1 = "300e894461d8a7cf21a4d2e4b49281e4f940a472";
    std::string commitHash2 = "4127b6c626f3fe9cb311b59c3b14ede9222c420b";
    changedFiles =
        diffAnalysis->getChangedFiles(repoPath, commitHash1, commitHash2);

    changedFiles = {"test_files/largest_check_multi_file/main.c",
                    "test_files/largest_check_multi_file/recur.c",
                    "test_files/largest_check_multi_file/recur.h",
                    "test_files/largest_check_multi_file/globals.h",
                    "test_files/largest_check_multi_file/largest_check.c",
                    "test_files/largest_check_multi_file/largest_check.h"};
    changedFiles = {"test_files/largest_check_multi_file/recur.c"};
    // changedFiles = {"test_files/largest_check_multi_file/largest_check.c"};

    changedFiles = {"test_files/Splash-4/altered/barnes/code.c"};
    // changedFiles +=
    //     fileIncludes->getChildren("test_files/Splash-4/altered/barnes/code.h");
  }

  debugCout << "Parsing changed files:" << std::endl;
  for (const auto &file : changedFiles) {
    debugCout << file << std::endl;
    callGraph->markNodesAsStale(file);
    parser->parseFile(file.c_str(), true);
  }
  debugCout << std::endl;

  std::vector<std::string> functions = parser->getFunctions();

  GraphVisualizer *visualizer = new GraphVisualizer();
  // visualizer->visualizeGraph(funcCfgs[functions[1]]);

  logTimeSinceLast("Parsing time: ", currTime);

  DeltaLockset *deltaLockset =
      new DeltaLockset(callGraph, parser, functionEraserSets);
  deltaLockset->updateLocksets(functions);

  logTimeSinceLast("Phase 1 time: ", currTime);

  FunctionVariableLocksets *functionVariableLocksets =
      new FunctionVariableLocksets(db);

  FunctionCumulativeLocksets *functionCumulativeLocksets =
      new FunctionCumulativeLocksets(db, functionVariableLocksets);

  VariableLocksets *variableLocksets =
      new VariableLocksets(callGraph, parser, functionVariableLocksets);

  CumulativeLocksets *cumulativeLocksets =
      new CumulativeLocksets(callGraph, functionCumulativeLocksets);

  variableLocksets->updateLocksets();
  logTimeSinceLast("Phase 2 time: ", currTime);
  cumulativeLocksets->updateLocksets();
  logTimeSinceLast("Phase 3 time: ", currTime);

  functionEraserSets->markFunctionEraserSetsAsOld();
  functionVariableLocksets->markFunctionVariableLocksetsAsOld();
  callGraph->deleteStaleNodes();

  std::set<std::string> dataRaces =
      functionCumulativeLocksets->detectDataRaces();

  // std::cout << "Variables with data races:" << std::endl;
  // for (const std::string &dataRace : dataRaces) {
  //   std::cout << dataRace << std::endl;
  // }

  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime)
          .count();

  std::cout << "Total time taken: " << duration << "ms" << std::endl;

  std::ifstream statFile("/proc/self/stat");
  std::string statLine;
  std::getline(statFile, statLine);
  std::istringstream iss(statLine);
  std::string entry;
  long long memUsage;
  for (int i = 1; i <= 24; i++) {
    std::getline(iss, entry, ' ');
    if (i == 24) {
      memUsage = stoi(entry);
    }
  }
  // std::cout << "Memory usage: " << (int)(4096 * memUsage / 1e6) << " MB"
  //           << std::endl;
  std::cout << std::endl;
}