#pragma once
#include "database.h"
#include "eraser_sets.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>

class FunctionEraserSets {
public:
  explicit FunctionEraserSets(Database *db);
  virtual ~FunctionEraserSets() = default;

  void combineSets(EraserSets &s1, EraserSets &s2);
  void combineSetsForRecursiveThreads(EraserSets &s1, EraserSets &s2);
  EraserSets *getEraserSets(std::string funcName);
  void updateCurrEraserSets(EraserSets &sets);
  void saveCurrEraserSets();
  void startNewFunction(std::string funcName);
  void markFunctionEraserSetsAsOld();
  void saveFunctionDirectVariableAccesses(std::set<std::string> &reads,
                                          std::set<std::string> &writes);
  void saveRecursiveUnlocks(std::set<std::string> &unlocks);

private:
  bool checkCurrFuncInDb();
  void insertCurrSetsIntoDb(bool locksChanged, bool varsChanged);
  void deleteCurrFuncFromDb();
  EraserSets extractSetsFromDb(std::string funcName);

  Database *db;
  std::unordered_map<std::string, EraserSets> functionSets;
  std::string currFunc;
  EraserSets currFuncSets;
  bool currFuncSetsStarted;
};