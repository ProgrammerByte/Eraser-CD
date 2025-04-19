#include "function_cumulative_locksets.h"

FunctionCumulativeLocksets::FunctionCumulativeLocksets(
    Database *db, FunctionVariableLocksets *functionVariableLocksets)
    : db(db), functionVariableLocksets(functionVariableLocksets){};

bool FunctionCumulativeLocksets::shouldVisitNode(std::string funcName) {
  sqlite3_stmt *stmt;
  std::string query = "SELECT 1 FROM function_variable_locksets WHERE funcname "
                      "= ? AND (recently_changed = 1 OR caller_locks_changed = "
                      "1 OR callee_locks_changed = 1);";
  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);
  bool result = sqlite3_step(stmt) == SQLITE_ROW;
  sqlite3_finalize(stmt);

  if (result) {
    return true;
  }

  query = "SELECT 1 FROM function_cumulative_locksets AS fvcl "
          "JOIN function_calls AS am ON am.callee = fvcl.funcname WHERE "
          "fvcl.recently_changed = 1 AND am.caller = ?;";

  db->prepareStatement(stmt, query, params);
  result = sqlite3_step(stmt) == SQLITE_ROW;
  sqlite3_finalize(stmt);
  return result;
}

std::set<std::string> FunctionCumulativeLocksets::getFunctionCumulativeAccesses(
    std::string funcName) {
  sqlite3_stmt *stmt;
  std::string query = "SELECT DISTINCT varname FROM "
                      "function_cumulative_accesses WHERE "
                      "funcname = ?;";
  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);

  std::set<std::string> variableAccesses = {};
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string varName = db->getStringFromStatement(stmt, 0);
    variableAccesses.insert(varName);
  }
  return variableAccesses;
}

TestVariableLocks FunctionCumulativeLocksets::getFunctionCumulativeLocksets(
    std::string funcName) {
  sqlite3_stmt *stmt;
  std::string query = "SELECT DISTINCT varname FROM "
                      "function_cumulative_accesses WHERE "
                      "funcname = ?;";
  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);
  VariableLocks defaultVariableLocks = {};
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string varName = db->getStringFromStatement(stmt, 0);
    defaultVariableLocks.insert({varName, {}});
  }

  query =
      "SELECT id, testname FROM function_cumulative_locksets WHERE "
      "funcname = ?;";
  db->prepareStatement(stmt, query, params);

  TestVariableLocks cumulativeLocksets;
  std::vector<std::string> ids = {};
  std::vector<std::string> testNames = {};
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    ids.push_back(db->getStringFromStatement(stmt, 0));
    std::string testName = db->getStringFromStatement(stmt, 1);
    testNames.push_back(testName);
    cumulativeLocksets.insert({testName, defaultVariableLocks});
  }
  sqlite3_finalize(stmt);

  query =
      "SELECT varname, lock FROM function_cumulative_locksets_outputs "
      "WHERE function_cumulative_locksets_id = ?;";
  for (int i = 0; i < ids.size(); i++) {
    params = {ids[i]};
    db->prepareStatement(stmt, query, params);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string varName = db->getStringFromStatement(stmt, 0);
      std::string lock = db->getStringFromStatement(stmt, 1);
      cumulativeLocksets[testNames[i]][varName].insert(lock);
    }
    sqlite3_finalize(stmt);
  }
  return cumulativeLocksets;
}

FunctionCumulativeData
FunctionCumulativeLocksets::getFunctionCumulativeData(std::string funcName) {
  return {getFunctionCumulativeLocksets(funcName),
          getFunctionCumulativeAccesses(funcName)};
}

std::set<std::string>
FunctionCumulativeLocksets::computeFunctionCumulativeAccesses(
    std::string funcName) {
  sqlite3_stmt *stmt;
  std::string query =
      "SELECT callee FROM function_calls WHERE caller = ?;";
  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);

  std::set<std::string> cumulativeAccesses = {};
  std::vector<std::string> callees = {};
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    callees.push_back(db->getStringFromStatement(stmt, 0));
  }
  sqlite3_finalize(stmt);

  query = "SELECT DISTINCT varname FROM function_cumulative_accesses "
          "WHERE funcname = ?;";
  for (const std::string &callee : callees) {
    params = {callee};
    db->prepareStatement(stmt, query, params);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      cumulativeAccesses.insert(db->getStringFromStatement(stmt, 0));
    }
    sqlite3_finalize(stmt);
  }

  query =
      "SELECT DISTINCT varname FROM function_variable_direct_accesses WHERE "
      "funcname = ?;";
  params = {funcName};
  db->prepareStatement(stmt, query, params);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    cumulativeAccesses.insert(db->getStringFromStatement(stmt, 0));
  }
  return cumulativeAccesses;
}

TestVariableLocks FunctionCumulativeLocksets::computeFunctionCumulativeLocksets(
    std::string funcName) {
  sqlite3_stmt *stmt;
  std::string query = "SELECT id, testname FROM function_variable_locksets "
                      "WHERE funcname = ?;";
  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);
  TestVariableLocks testVariableLocks = {};
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string id = db->getStringFromStatement(stmt, 0);
    std::string testName = db->getStringFromStatement(stmt, 1);

    testVariableLocks.insert(
        {testName, functionVariableLocksets->getVariableLocks(funcName, id)});
  }

  query = "SELECT callee FROM function_calls WHERE caller = ?;";
  params = {funcName};
  db->prepareStatement(stmt, query, params);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string callee = db->getStringFromStatement(stmt, 0);
    testVariableLocks *= getFunctionCumulativeLocksets(callee);
  }
  return testVariableLocks;
}

FunctionCumulativeData
FunctionCumulativeLocksets::computeFunctionCumulativeData(
    std::string funcName) {
  return {computeFunctionCumulativeLocksets(funcName),
          computeFunctionCumulativeAccesses(funcName)};
}

void FunctionCumulativeLocksets::deleteFunctionCumulativeData(
    std::string funcName) {
  sqlite3_stmt *stmt;
  std::string query =
      "DELETE FROM function_cumulative_locksets WHERE funcname = ?;";
  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);

  query =
      "DELETE FROM function_cumulative_accesses WHERE funcname = ?;";
  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);
}

void FunctionCumulativeLocksets::insertFunctionCumulativeData(
    std::string funcName, FunctionCumulativeData functionCumulativeData) {
  TestVariableLocks testVariableLocks = functionCumulativeData.locksets;
  std::set<std::string> variableAccesses =
      functionCumulativeData.variableAccesses;

  sqlite3_stmt *stmt;
  std::string query = "INSERT INTO function_cumulative_locksets "
                      "(funcname, testname) VALUES (?, ?);";
  std::vector<std::string> params;
  for (const auto &pair : testVariableLocks) {
    params = {funcName, pair.first};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }

  query = "SELECT id FROM function_cumulative_locksets WHERE "
          "funcname = ? AND testname = ?;";
  for (const auto &pair : testVariableLocks) {
    params = {funcName, pair.first};
    db->prepareStatement(stmt, query, params);
    std::string id;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      id = db->getStringFromStatement(stmt, 0);
    }
    sqlite3_finalize(stmt);

    query = "INSERT INTO function_cumulative_locksets_outputs "
            "(function_cumulative_locksets_id, varname, lock) "
            "VALUES (?, ?, ?);";
    for (const auto &pair2 : pair.second) {
      for (const std::string &lock : pair2.second) {
        params = {id, pair2.first, lock};
        db->prepareStatement(stmt, query, params);
        db->runStatement(stmt);
      }
    }
  }

  query = "INSERT INTO function_cumulative_accesses (funcname, "
          "varname, type) VALUES (?, ?, ?);";
  for (const std::string &varName : variableAccesses) {
    params = {funcName, varName, "read"};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }
}

void FunctionCumulativeLocksets::updateFunctionCumulativeLocksets(
    std::string funcName) {
  FunctionCumulativeData originalCumulativeData =
      getFunctionCumulativeData(funcName);
  FunctionCumulativeData newCumulativeData =
      computeFunctionCumulativeData(funcName);

  if (originalCumulativeData != newCumulativeData) {
    deleteFunctionCumulativeData(funcName);
    insertFunctionCumulativeData(funcName, newCumulativeData);
  }
}

std::vector<std::string> FunctionCumulativeLocksets::getFunctionsForTesting() {
  sqlite3_stmt *stmt;
  std::string query = "SELECT funcname FROM function_variable_locksets WHERE "
                      "recently_changed = 1";

  db->prepareStatement(stmt, query);
  std::vector<std::string> functions = {};
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    functions.push_back(db->getStringFromStatement(stmt, 0));
  }
  return functions;
}

std::set<std::string> FunctionCumulativeLocksets::detectDataRaces() {
  std::string funcName = "main";
  std::string testName = "debug";

  VariableLocks mainLocksets =
      getFunctionCumulativeLocksets(funcName)[testName];

  std::set<std::string> dataRaces = {};
  sqlite3_stmt *stmt;
  std::string query = "SELECT varname FROM function_vars WHERE funcname = ? "
                      "AND type = 'shared_modified';";
  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string varName = db->getStringFromStatement(stmt, 0);
    if (mainLocksets.find(varName) == mainLocksets.end() ||
        mainLocksets[varName].empty()) {
      dataRaces.insert(varName);
    }
  }
  return dataRaces;
}