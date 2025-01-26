#include "function_variable_locksets.h"

FunctionVariableLocksets::FunctionVariableLocksets(Database *db) : db(db) {
  functionLocks = {};
  currFunc = "";
  currId = "";
};

std::string FunctionVariableLocksets::getId(std::string funcName,
                                            std::string testName) {
  sqlite3_stmt *stmt;
  std::string query = "SELECT id FROM function_variable_locksets WHERE "
                      "funcname = ? AND testname = ?;";
  std::vector<std::string> params = {funcName, testName};
  db->prepareStatement(stmt, query, params);
  std::string id;

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
  }
  sqlite3_finalize(stmt);

  if (id == "") {
    query = "INSERT INTO function_variable_locksets (funcname, testname) "
            "VALUES (?, ?);";
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);

    query = "SELECT id FROM function_variable_locksets WHERE funcname = ? AND "
            "testname = ?;";
    db->prepareStatement(stmt, query, params);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    }
  }
  return id;
}

void FunctionVariableLocksets::startNewFunction(std::string funcName) {
  currFunc = funcName;
}

void FunctionVariableLocksets::startNewTest(std::string testName) {
  currTest = testName;
  currFuncSets = {};
  currId = getId(currFunc, testName);
}

void FunctionVariableLocksets::extractFunctionLocksFromDb(
    std::string funcName, std::set<std::string> &dbLocks,
    std::set<std::string> &dbUnlocks) {
  sqlite3_stmt *stmt;
  std::string query = "SELECT * FROM function_locks WHERE funcname = ?;";
  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string varName =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    std::string type =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    if (type == "lock") {
      dbLocks.insert(varName);
    } else {
      dbUnlocks.insert(varName);
    }
  }
  sqlite3_finalize(stmt);

  functionLocks.insert({funcName, dbLocks});
  functionUnlocks.insert({funcName, dbUnlocks});
}

void FunctionVariableLocksets::applyDeltaLockset(std::set<std::string> &locks,
                                                 std::string funcName) {
  std::set<std::string> dbLocks;
  std::set<std::string> dbUnlocks;
  if (functionLocks.find(funcName) != functionLocks.end()) {
    dbLocks = functionLocks[funcName];
    dbUnlocks = functionUnlocks[funcName];
  } else {
    extractFunctionLocksFromDb(funcName, dbLocks, dbUnlocks);
  }
  locks += dbLocks;
  locks -= dbUnlocks;
}

FunctionInputs FunctionVariableLocksets::updateAndCheckCombinedInputs() {
  FunctionInputs functionInputs = {{}, {}};
  // TODO - HARD CODED TEST!!!
  if (currFunc == "main") {
    functionInputs.changedTests.insert({"debug", {}});
    functionInputs.reachableTests = {"debug"};
    return functionInputs;
  }

  sqlite3_stmt *stmt;
  // TODO - MAYBE recently_changed FOR FUNCTION CHANGED AND A SEPARATE
  // inputs_changed?
  std::string query = "SELECT id, testname, recently_changed FROM "
                      "function_variable_locksets WHERE funcname = ?;";
  std::vector<std::string> params = {currFunc};
  db->prepareStatement(stmt, query, params);
  std::vector<std::string> ids = {};
  std::vector<std::string> testnames = {};
  std::vector<bool> recentlyChanged = {};
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    ids.push_back(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
    testnames.push_back(
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
    recentlyChanged.push_back(db->retrieveBoolean(
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2))));
  }
  sqlite3_finalize(stmt);
  functionInputs.reachableTests = testnames;

  for (int i = 0; i < ids.size(); i++) {
    std::string id = ids[i];
    std::string testname = testnames[i];
    bool changed = recentlyChanged[i];
    std::set<std::string> oldCombinedLocks = {};
    if (changed) {
      query = "DELETE FROM function_variable_locksets_combined_inputs WHERE "
              "function_variable_locksets_id = ?;";
      params = {id};
      db->prepareStatement(stmt, query, params);
      db->runStatement(stmt);
    } else {
      query =
          "SELECT lock FROM function_variable_locksets_combined_inputs WHERE "
          "function_variable_locksets_id = ?;";
      params = {id};
      db->prepareStatement(stmt, query, params);
      while (sqlite3_step(stmt) == SQLITE_ROW) {
        oldCombinedLocks.insert(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
      }
      sqlite3_finalize(stmt);
    }

    std::unordered_map<std::string, std::set<std::string>> callerLocksets = {};
    std::vector<std::string> callers = {};

    query = "SELECT funcname1 FROM adjacency_matrix AS am JOIN "
            "function_variable_locksets AS fvl ON am.funcname2 = fvl.funcname "
            "WHERE fvl.id = ?;";

    params = {id};
    db->prepareStatement(stmt, query, params);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string caller =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      if (callerLocksets.find(caller) == callerLocksets.end()) {
        callerLocksets.insert({caller, {}});
        callers.push_back(caller);
      }
    }
    sqlite3_finalize(stmt);

    if (callers.empty()) {
      query = "DELETE FROM function_variable_locksets_combined_inputs WHERE "
              "function_variable_locksets_id = ?;";
      params = {id};
      db->prepareStatement(stmt, query, params);
      db->runStatement(stmt);

      if (changed || !oldCombinedLocks.empty()) {
        functionInputs.changedTests.insert({testname, {}});
      }
      continue;
    }

    query =
        "SELECT caller, lock FROM function_variable_locksets_callers AS fvlc "
        "JOIN function_variable_locksets_callers_locks AS fvlcl ON fvlc.id = "
        "fvlcl.function_variable_locksets_callers_id WHERE "
        "function_variable_locksets_id = ?;";
    params = {id};
    db->prepareStatement(stmt, query, params);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string caller =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      std::string lock =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
      callerLocksets[caller].insert(lock);
    }
    sqlite3_finalize(stmt);

    std::set<std::string> newCombinedLocks = callerLocksets[callers[0]];
    for (int i = 1; i < callers.size(); i++) {
      newCombinedLocks *= callerLocksets[callers[i]];
    }

    if (changed || newCombinedLocks != oldCombinedLocks) {
      query = "DELETE FROM function_variable_locksets_combined_inputs WHERE "
              "function_variable_locksets_id = ?;";
      params = {id};
      db->prepareStatement(stmt, query, params);
      db->runStatement(stmt);

      query = "INSERT INTO function_variable_locksets_combined_inputs "
              "(function_variable_locksets_id, lock) VALUES (?, ?);";
      for (const std::string &lock : newCombinedLocks) {
        params = {id, lock};
        db->prepareStatement(stmt, query, params);
        db->runStatement(stmt);
      }
      functionInputs.changedTests.insert({testname, newCombinedLocks});
    }
  }
  return functionInputs;
}

bool FunctionVariableLocksets::shouldVisitNode(std::string funcName) {
  sqlite3_stmt *stmt;
  std::string query =
      "SELECT 1 FROM function_variable_locksets WHERE funcname "
      "= ? AND (recently_changed = 0 AND caller_locks_changed = 0);";

  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);
  bool result = sqlite3_step(stmt) == SQLITE_ROW;
  sqlite3_finalize(stmt);
  return !result;
}

void FunctionVariableLocksets::addFuncCallLocksets(
    std::unordered_map<std::string, std::set<std::string>> funcCallLocksets) {
  sqlite3_stmt *stmt;
  std::string query;
  std::vector<std::string> params;

  query = "SELECT fvlc.id FROM function_variable_locksets_callers AS fvlc JOIN "
          "function_variable_locksets AS fvl ON fvl.id = "
          "fvlc.function_variable_locksets_id WHERE caller = "
          "? AND testname = ?;";
  params = {currFunc, currTest};
  db->prepareStatement(stmt, query, params);
  std::vector<std::string> ids = {};
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    ids.push_back(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
  }
  sqlite3_finalize(stmt);

  for (const std::string &id : ids) {
    query = "DELETE FROM function_variable_locksets_callers WHERE id = ?";
    params = {id};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }

  for (const auto &pair : funcCallLocksets) {
    std::string funcName = pair.first;
    std::set<std::string> locks = pair.second;

    std::string funcId = getId(funcName, currTest);
    query = "UPDATE function_variable_locksets SET caller_locks_changed = 1 "
            "WHERE id = ?;";
    params = {funcId};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);

    query = "INSERT INTO function_variable_locksets_callers "
            "(function_variable_locksets_id, caller) VALUES (?, ?)";
    params = {funcId, currFunc};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);

    query =
        "SELECT fvlc.id FROM function_variable_locksets_callers AS fvlc JOIN "
        "function_variable_locksets AS fvl ON fvl.id = "
        "fvlc.function_variable_locksets_id WHERE caller = "
        "? AND testname = ?;";
    params = {currFunc, currTest};
    db->prepareStatement(stmt, query, params);
    std::string id;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    }

    query = "INSERT OR IGNORE INTO function_variable_locksets_callers_locks "
            "(function_variable_locksets_callers_id, lock) VALUES (?, ?);";
    for (const std::string &lock : locks) {
      params = {id, lock};
      db->prepareStatement(stmt, query, params);
      db->runStatement(stmt);
    }
  }
}

void FunctionVariableLocksets::addVariableLocksets(
    std::unordered_map<std::string, std::set<std::string>> variableLocksets) {
  sqlite3_stmt *stmt;
  std::string query;
  std::vector<std::string> params;

  for (const auto &pair : variableLocksets) {
    std::string varName = pair.first;
    std::set<std::string> locks = pair.second;

    query = "DELETE FROM function_variable_locksets_outputs WHERE "
            "function_variable_locksets_id = ?";
    params = {currId};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);

    query = "INSERT INTO function_variable_locksets_outputs "
            "(function_variable_locksets_id, varname, lock) VALUES (?, ?, ?);";
    for (const std::string &lock : locks) {
      params = {currId, varName, lock};
      db->prepareStatement(stmt, query, params);
      db->runStatement(stmt);
    }
  }
}

VariableLocks FunctionVariableLocksets::getVariableLocks() {
  sqlite3_stmt *stmt;
  std::string query =
      "SELECT varname FROM function_variable_direct_accesses WHERE "
      "funcname = ?;";
  std::vector<std::string> params = {currFunc};
  db->prepareStatement(stmt, query, params);
  VariableLocks variableLocks;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string varName =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    if (variableLocks.find(varName) == variableLocks.end()) {
      variableLocks.insert({varName, {}});
    }
  }
  sqlite3_finalize(stmt);

  query = "SELECT varname, lock FROM function_variable_locksets_outputs WHERE "
          "function_variable_locksets_id = ?;";
  params = {currId};
  db->prepareStatement(stmt, query, params);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string varName =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    std::string lock =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    variableLocks[varName].insert(lock);
  }
  sqlite3_finalize(stmt);

  return variableLocks;
}

VariableLocks FunctionVariableLocksets::getVariableLocks(std::string func,
                                                         std::string id) {
  currFunc = func;
  currId = id;
  return getVariableLocks();
}

void FunctionVariableLocksets::markFunctionVariableLocksetsAsOld() {
  sqlite3_stmt *stmt;
  std::string query = "UPDATE function_variable_locksets SET recently_changed "
                      "= 0, caller_locks_changed = 0;";
  db->prepareStatement(stmt, query);
  db->runStatement(stmt);

  query =
      "UPDATE function_variable_cumulative_locksets SET recently_changed = 0;";
  db->prepareStatement(stmt, query);
  db->runStatement(stmt);
}

std::set<std::string> FunctionVariableLocksets::getFunctionRecursiveUnlocks() {
  sqlite3_stmt *stmt;
  std::string query =
      "SELECT varname FROM function_recursive_unlocks WHERE funcname = ?;";
  std::vector<std::string> params = {currFunc};
  db->prepareStatement(stmt, query, params);
  std::set<std::string> unlocks;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string varName =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    unlocks.insert(varName);
  }
  sqlite3_finalize(stmt);
  return unlocks;
}