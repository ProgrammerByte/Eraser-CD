#include "call_graph.h"
#include "construction_environment.h"
#include "cumulative_locksets.h"
#include "database.h"
#include "debug_tools.h"
#include "delta_lockset.h"
#include "diff_analysis.h"
#include "eraser_settings.h"
#include "file_includes.h"
#include "function_cumulative_locksets.h"
#include "function_variable_locksets.h"
#include "graph_visualizer.h"
#include "parser.h"
#include "variable_locksets.h"
#include <chrono>
#include <clang-c/Index.h>
#include <filesystem>
#include <iostream>
#include <sys/resource.h>
#include <unordered_map>
#include <vector>
#include <fstream>

bool saveTimes = false;
std::ofstream outFile;

void logTimeSinceLast(
    std::string label,
    std::chrono::time_point<std::chrono::high_resolution_clock> &currTime) {
  auto nextTime = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(nextTime - currTime)
          .count();
  
  if (saveTimes) {
    outFile << duration << " ";
  }
  std::cout << label << duration << "ms" << std::endl;
  currTime = nextTime;
}

bool fileExists(const std::string &filename) {
  std::ifstream file(filename);
  return file.good();
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cout
        << "Expected usage: static_eraser <path> <commit_hash> <initial_commit> <simplified_output>"
        << std::endl;
    return 0;
  }
  std::string repoPath = argv[1];
  std::string currHash = argv[2];

  bool initialCommit = (std::string(argv[3]) == "y" || std::string(argv[3]) == "Y") ||
                       !fileExists(Database::dbName);

  saveTimes = (std::string(argv[4]) == "y" || std::string(argv[4]) == "Y");

  if (saveTimes) {
    outFile.open("timing_output.txt");
    if (!outFile.is_open()) {
      std::cerr << "Failed to open file for writing." << std::endl;
      return 1;
    }
  }

  auto startTime = std::chrono::high_resolution_clock::now();
  auto currTime = startTime;

  Database db(initialCommit);
  FunctionEraserSets functionEraserSets(&db);
  CallGraph callGraph(&db);
  FileIncludes fileIncludes(&db);
  EraserSettings eraserSettings(&db);
  DiffAnalysis diffAnalysis(&fileIncludes);
  Parser parser(&callGraph, &fileIncludes);

  std::string prevHash = eraserSettings.getAndUpdatePrevHash(currHash);
  std::set<std::string> changedFiles;
  if (initialCommit) {
    changedFiles = diffAnalysis.getAllFiles(repoPath);
  } else {
    changedFiles = diffAnalysis.getChangedFiles(repoPath, prevHash, currHash);
  }

  // std::string repoPath = "~/dissertation/Eraser-CD";
  // std::string repoPath =
  //     "test_files/Splash-3/"
  //     "e35efba59688a585275d19a16bc6f9371da978e0/ocean_non_contiguous";
  /*
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

    // std::string repoPath = "test_files/Splash-4/altered/barnes";
    // std::string repoPath = "test_files/Splash-4/altered/cholesky";
    // std::string repoPath =
    //     "test_files/Splash-4/altered/ocean-non_contiguous_partitions";
    changedFiles = diffAnalysis.getAllFiles(repoPath);
  } else {
    std::string commitHash1 = "300e894461d8a7cf21a4d2e4b49281e4f940a472";
    std::string commitHash2 = "4127b6c626f3fe9cb311b59c3b14ede9222c420b";
    changedFiles =
        diffAnalysis.getChangedFiles(repoPath, commitHash1, commitHash2);

    changedFiles = {"test_files/largest_check_multi_file/main.c",
                    "test_files/largest_check_multi_file/recur.c",
                    "test_files/largest_check_multi_file/recur.h",
                    "test_files/largest_check_multi_file/globals.h",
                    "test_files/largest_check_multi_file/largest_check.c",
                    "test_files/largest_check_multi_file/largest_check.h"};
    changedFiles = {"test_files/largest_check_multi_file/recur.c"};
    // changedFiles = {"test_files/largest_check_multi_file/largest_check.c"};

    changedFiles = {"test_files/Splash-4/altered/barnes/code.c"};
    // changedFiles = {"test_files/Splash-4/altered/fft.c"};
    // changedFiles = {};
    // changedFiles +=
    // fileIncludes->getChildren("test_files/Splash-4/altered/barnes/code.h");
    changedFiles = {
        "test_files/Splash-4/altered/ocean-non_contiguous_partitions/main.c"};
    // changedFiles = {
    //     "test_files/Splash-4/altered/ocean-non_contiguous_partitions/main.c",
    // "test_files/Splash-4/altered/ocean-non_contiguous_partitions/slave1.c",
    // "test_files/Splash-4/altered/ocean-non_contiguous_partitions/slave2.c"};

    // changedFiles = {"test_files/Splash-4/altered/cholesky/solve.c"};

    // ocean non contiguous testing Splash-3

    // second commit:
    // changedFiles = {repoPath + "/main.c", repoPath + "/slave1.c"};

    // third, fourth, fifth, sixth commit:
    // changedFiles = {repoPath + "/main.c", repoPath + "/decs.h"};
    // changedFiles = diffAnalysis.getAllFiles(repoPath);

    // seventh commit
    changedFiles = {repoPath + "/slave2.c"};

    // volrend testing Splash-3

    // second commit:
    // changedFiles = {repoPath + "/adaptive.c", repoPath + "/anl.h",
    //                 repoPath + "/file.c", repoPath + "/main.c"};

    // third commit:
    // changedFiles = {repoPath + "/normal.c", repoPath + "/octree.c",
    //                 repoPath + "/opacity.c"};

    // fourth commit:
    // changedFiles = {repoPath + "/const.h", repoPath + "/global.h",
    //                 repoPath + "/main.c", repoPath + "/user_options.h"};

    // fifth commit:
    // changedFiles = {repoPath + "/adaptive.c", repoPath + "/anl.h",
    //                 repoPath + "/main.c",     repoPath + "/normal.c",
    //                 repoPath + "/octree.c",   repoPath + "/opacity.c"};

    // sixth and seventh commit:
    // changedFiles = {repoPath + "/main.c"};

    // barnes testing Splash-3

    // second commit:
    // changedFiles = {
    //     repoPath + "/barnes/code.c",      repoPath + "/barnes/code.h",
    //     repoPath + "/barnes/defs.h",      repoPath + "/barnes/getparam.c",
    //     repoPath + "/barnes/grav.c",      repoPath + "/barnes/load.c",
    //     repoPath + "/barnes/stdinc_pre.h"};

    // third commit:
    // changedFiles = {repoPath + "/barnes/code.c", repoPath +
    // "/barnes/grav.c"};

    // fourth commit:
    // changedFiles = {repoPath + "/barnes/defs.h"};

    // fifth commit:
    // changedFiles = {repoPath + "/barnes/load.c", repoPath +
    // "/barnes/code_io.c",
    //                 repoPath + "/barnes/code.h", repoPath +
    //                 "/barnes/code.c"};

    // sixth commit:
    // changedFiles = {repoPath + "/barnes/code.c"};

    // seventh commit:
    // changedFiles = {repoPath + "/barnes/load.c"};
  }
    */

  // changedFiles = {"test_files/Splash-4/altered/cholesky/amal.c"};
  debugCout << "Parsing changed files:" << std::endl;
  for (const auto &file : changedFiles) {
    debugCout << file << std::endl;
    callGraph.markNodesAsStale(file);
    parser.parseFile(file.c_str(), true);
  }
  debugCout << std::endl;

  std::vector<std::string> functions = parser.getFunctions();

  GraphVisualizer visualizer;
  // visualizer.visualizeGraph(funcCfgs["ConsiderMerge"]);

  logTimeSinceLast("Parsing time: ", currTime);

  DeltaLockset deltaLockset(&callGraph, &parser, &functionEraserSets);
  deltaLockset.updateLocksets(functions);

  logTimeSinceLast("Phase 1 time: ", currTime);

  FunctionVariableLocksets functionVariableLocksets(&db);

  FunctionCumulativeLocksets functionCumulativeLocksets(
      &db, &functionVariableLocksets);

  VariableLocksets variableLocksets(&callGraph, &parser,
                                    &functionVariableLocksets);

  CumulativeLocksets cumulativeLocksets(&callGraph,
                                        &functionCumulativeLocksets);

  variableLocksets.updateLocksets();
  logTimeSinceLast("Phase 2 time: ", currTime);
  cumulativeLocksets.updateLocksets();
  logTimeSinceLast("Phase 3 time: ", currTime);

  functionEraserSets.markFunctionEraserSetsAsOld();
  functionVariableLocksets.markFunctionVariableLocksetsAsOld();
  callGraph.deleteStaleNodes();

  std::set<std::string> dataRaces =
      functionCumulativeLocksets.detectDataRaces();

  std::cout << "Variables with data races:" << std::endl;
  for (const std::string &dataRace : dataRaces) {
    std::cout << dataRace << std::endl;
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime)
          .count();

  if (saveTimes) {
    outFile << duration << " ";
  }
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
  if (saveTimes) {
    outFile << (int)(4096 * memUsage / 1e3);
  }
  std::cout << "Memory usage: " << (int)(4096 * memUsage / 1e3) << " KB"
            << std::endl;
  std::cout << std::endl;
  if (outFile.is_open()) {
    outFile.close();
}
}