#pragma once
#include "database.h"
#include "function_variable_locksets.h"
#include "variable_locks.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <set>
#include <sqlite3.h>
#include <string>
#include <unordered_map>
#include <vector>

struct TestVariableLocks
    : public std::unordered_map<std::string, VariableLocks> {
  TestVariableLocks &operator*=(const TestVariableLocks &other) {
    for (auto it = begin(); it != end();) {
      if (other.find(it->first) != other.end()) {
        it->second *= other.at(it->first);
        ++it;
      } else {
        it = erase(it);
      }
    }
    return *this;
  }

  TestVariableLocks operator*(const TestVariableLocks &other) const {
    TestVariableLocks result = *this;
    return result *= other;
  }
};

struct FunctionCumulativeData {
  TestVariableLocks locksets;
  std::set<std::string> variableAccesses;

  bool operator==(const FunctionCumulativeData &other) const {
    return locksets == other.locksets &&
           variableAccesses == other.variableAccesses;
  }

  bool operator!=(const FunctionCumulativeData &other) const {
    return !(*this == other);
  }
};

class FunctionCumulativeLocksets {
public:
  explicit FunctionCumulativeLocksets(
      Database *db, FunctionVariableLocksets *functionVariableLocksets);
  virtual ~FunctionCumulativeLocksets() = default;
  bool shouldVisitNode(std::string funcName);
  void updateFunctionCumulativeLocksets(std::string funcName);
  std::set<std::string> detectDataRaces();

private:
  std::set<std::string> getFunctionCumulativeAccesses(std::string funcName);
  TestVariableLocks getFunctionCumulativeLocksets(std::string funcName);
  FunctionCumulativeData getFunctionCumulativeData(std::string funcName);
  std::set<std::string> computeFunctionCumulativeAccesses(std::string funcName);
  TestVariableLocks computeFunctionCumulativeLocksets(std::string funcName);
  FunctionCumulativeData computeFunctionCumulativeData(std::string funcName);
  void deleteFunctionCumulativeData(std::string funcName);
  void
  insertFunctionCumulativeData(std::string funcName,
                               FunctionCumulativeData functionCumulativeData);
  Database *db;
  FunctionVariableLocksets *functionVariableLocksets;
};