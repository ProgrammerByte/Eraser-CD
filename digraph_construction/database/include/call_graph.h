#pragma once
#include "database.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>

class CallGraph {
public:
  explicit CallGraph(Database *db);
  virtual ~CallGraph() = default;
  void addNode(std::string funcName, std::string fileName);
  void addEdge(std::string caller, std::string callee, bool onThread);
  std::vector<std::string>
  deltaLocksetOrdering(std::vector<std::string> functions);
  std::vector<std::string>
  functionVariableLocksetsOrdering(std::vector<std::string> functions);
  bool shouldVisitNode(std::string funcName);

private:
  Database *db;
  std::vector<std::string> traverseGraph(bool reverse);
  std::vector<std::string> getNextNodes(std::vector<std::string> &order);
  void markNodes(std::vector<std::string> &startNodes, bool reverse);
};