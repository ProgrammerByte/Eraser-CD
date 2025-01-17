#pragma once
#include "database.h"
#include "variable_locks.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>

struct FunctionInputs {
  std::vector<std::string> reachableTests;
  std::unordered_map<std::string, std::set<std::string>> changedTests;
};

class FunctionVariableLocksets {
public:
  explicit FunctionVariableLocksets(Database *db);
  virtual ~FunctionVariableLocksets() = default;

  void startNewFunction(std::string funcName);
  void startNewTest(std::string testName);
  void applyDeltaLockset(std::set<std::string> &locks, std::string funcName);
  FunctionInputs updateAndCheckCombinedInputs();
  bool shouldVisitNode(std::string funcName);
  void addFuncCallLocksets(
      std::unordered_map<std::string, std::set<std::string>> funcCallLocksets);
  void addVariableLocksets(
      std::unordered_map<std::string, std::set<std::string>> variableLocksets);
  VariableLocks getVariableLocks();
  VariableLocks getVariableLocks(std::string func, std::string id);
  void markFunctionVariableLocksetsAsOld();
  std::set<std::string> getFunctionRecursiveUnlocks();

private:
  void extractFunctionLocksFromDb(std::string funcName,
                                  std::set<std::string> &dbLocks,
                                  std::set<std::string> &dbUnlocks);
  std::string getId(std::string funcName, std::string testName);

  Database *db;
  std::unordered_map<std::string, VariableLocks> functionSets;
  std::unordered_map<std::string, std::set<std::string>> functionLocks;
  std::unordered_map<std::string, std::set<std::string>> functionUnlocks;
  std::string currFunc;
  std::string currTest;
  std::string currId;
  VariableLocks currFuncSets;
};