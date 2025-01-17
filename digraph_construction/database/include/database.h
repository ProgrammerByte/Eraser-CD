#pragma once
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>

class Database {
public:
  explicit Database();
  virtual ~Database();

  void prepareStatement(sqlite3_stmt *&stmt, std::string query,
                        std::vector<std::string> &params);
  void prepareStatement(sqlite3_stmt *&stmt, std::string query);
  void runStatement(sqlite3_stmt *stmt);
  void deleteDatabase();
  void createTable(std::string query, std::string tableName);
  void createTables();

  std::string createTupleList(std::vector<std::string> &nodes);
  std::string createBoolean(bool value);
  bool retrieveBoolean(std::string value);

private:
  std::string dbName = "eraser.db";
  char *errMsg = 0;
  sqlite3 *db;
};