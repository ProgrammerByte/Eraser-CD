#pragma once
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>

class CallGraph {
public:
  explicit CallGraph();
  virtual ~CallGraph();
  void addNode(std::string funcName);
  void addEdge(std::string caller, std::string callee);

private:
  std::string dbName = "eraser.db";
  char *errMsg = 0;
  sqlite3 *db;

  void prepareStatement(sqlite3_stmt *&stmt, std::string query,
                        std::vector<std::string> &params);
  void prepareStatement(sqlite3_stmt *&stmt, std::string query);
  void runStatement(sqlite3_stmt *stmt);
  void deleteDatabase();
  void createTable(std::string query, std::string tableName);
  void createTables();
};