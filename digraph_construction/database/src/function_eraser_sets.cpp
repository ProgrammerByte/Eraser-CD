#include "function_eraser_sets.h"

FunctionEraserSets::FunctionEraserSets(Database *db) : db(db) {
  functionSets = {};
  currFunc = "";
};

void FunctionEraserSets::combineSets(EraserSets &s1, EraserSets &s2) {
  s1.locks *= s2.locks;
  s1.unlocks += s2.unlocks;
  s1.sharedModified += s2.sharedModified;
  // TODO - nit: var can be in both internal and external shared at once if
  // tracking independently, although not the biggest thing
  s1.internalShared += s2.internalShared - s1.sharedModified;
  s1.externalShared += s2.externalShared - s1.sharedModified;
  std::set<std::string> sOrSm =
      s1.sharedModified + s1.internalShared + s1.externalShared; // TODO -
  //    CONSIDER USING THIS!!!
  // std::set<std::string> sOrSm = s1.sharedModified;
  s1.externalReads += s2.externalReads;
  s1.externalReads -= sOrSm;
  s1.externalWrites += s2.externalWrites;
  s1.externalWrites -= sOrSm;
  s1.internalReads += s2.internalReads;
  s1.internalReads -= sOrSm;
  s1.internalWrites += s2.internalWrites;
  s1.internalWrites -= sOrSm;
  s1.queuedWrites += s2.queuedWrites;
  s1.queuedWrites.removeVars(s1.sharedModified);
  s1.activeThreads += s2.activeThreads;
  s1.finishedThreads *= s2.finishedThreads;
}

void FunctionEraserSets::combineSetsForRecursiveThreads(EraserSets &s1,
                                                        EraserSets &s2) {
  s1.sharedModified += s2.sharedModified;
  s1.externalShared += s2.internalShared + s2.externalShared;
  // std::set<std::string> sOrSm =
  //    s1.sharedModified + s1.internalShared + s1.externalShared; // TODO -
  //    CONSIDER USING THIS!!!
  std::set<std::string> sOrSm = s1.sharedModified;
  s1.externalReads += s2.internalReads + s2.externalReads;
  s1.externalReads -= sOrSm;
  s1.externalWrites += s2.internalWrites + s2.externalWrites;
  s1.externalWrites -= sOrSm;
  s1.queuedWrites += s2.queuedWrites;
  s1.queuedWrites.removeVars(s1.sharedModified);
  s1.activeThreads += s2.activeThreads;
}

bool FunctionEraserSets::checkCurrFuncInDb() {
  sqlite3_stmt *stmt;
  std::string query =
      "SELECT 1 FROM function_eraser_sets WHERE funcname = ? LIMIT 1;";

  std::vector<std::string> params = {currFunc};
  db->prepareStatement(stmt, query, params);
  bool result = sqlite3_step(stmt) == SQLITE_ROW;
  sqlite3_finalize(stmt);
  return result;
}

void FunctionEraserSets::insertCurrSetsIntoDb() {
  sqlite3_stmt *stmt;
  std::string query = "INSERT INTO function_eraser_sets (funcname) VALUES (?);";
  std::vector<std::string> params = {currFunc};
  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);

  query =
      "INSERT INTO function_locks (funcname, varname, type) VALUES (?, ?, ?);";
  for (const std::string &lock : currFuncSets.locks) {
    params = {currFunc, lock, "lock"};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }
  for (const std::string &unlock : currFuncSets.unlocks) {
    params = {currFunc, unlock, "unlock"};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }

  query =
      "INSERT INTO function_vars (funcname, varname, type) VALUES (?, ?, ?);";
  for (const std::string &read : currFuncSets.externalReads) {
    params = {currFunc, read, "external_read"};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }
  for (const std::string &read : currFuncSets.internalReads) {
    params = {currFunc, read, "internal_read"};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }
  for (const std::string &write : currFuncSets.externalWrites) {
    params = {currFunc, write, "external_write"};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }
  for (const std::string &write : currFuncSets.internalWrites) {
    params = {currFunc, write, "internal_write"};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }
  for (const std::string &shared : currFuncSets.internalShared) {
    params = {currFunc, shared, "internal_shared"};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }
  for (const std::string &shared : currFuncSets.externalShared) {
    params = {currFunc, shared, "external_shared"};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }
  for (const std::string &shared : currFuncSets.sharedModified) {
    params = {currFunc, shared, "shared_modified"};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }

  query =
      "INSERT INTO queued_writes (funcname, tid, varname) VALUES (?, ?, ?);";
  for (const auto &pair : currFuncSets.queuedWrites) {
    for (const std::string &write : pair.second) {
      params = {currFunc, pair.first, write};
      db->prepareStatement(stmt, query, params);
      db->runStatement(stmt);
    }
  }

  query = "INSERT INTO finished_threads (funcname, varname) VALUES (?, ?);";
  for (const std::string &thread : currFuncSets.finishedThreads) {
    params = {currFunc, thread};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }

  query =
      "INSERT INTO active_threads (funcname, varname, tid) VALUES (?, ?, ?);";
  for (const auto &pair : currFuncSets.activeThreads) {
    for (const std::string &tid : pair.second) {
      params = {currFunc, pair.first, tid};
      db->prepareStatement(stmt, query, params);
      db->runStatement(stmt);
    }
  }
}

void FunctionEraserSets::deleteCurrFuncFromDb() {
  sqlite3_stmt *stmt;
  std::string query = "DELETE FROM function_eraser_sets WHERE funcname = ?;";

  std::vector<std::string> params = {currFunc};
  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);
}

EraserSets FunctionEraserSets::extractSetsFromDb(std::string funcName) {
  EraserSets sets = EraserSets::defaultValue;
  sqlite3_stmt *stmt;

  std::string query = "SELECT * FROM function_locks WHERE funcname = ?;";
  std::vector<std::string> params = {currFunc};
  db->prepareStatement(stmt, query, params);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string varName =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    std::string type =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    if (type == "lock") {
      sets.locks.insert(varName);
    } else {
      sets.unlocks.insert(varName);
    }
  }
  sqlite3_finalize(stmt);

  query = "SELECT * FROM function_vars WHERE funcname = ?;";
  db->prepareStatement(stmt, query, params);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string varName =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    std::string type =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    if (type == "external_read") {
      sets.externalReads.insert(varName);
    } else if (type == "internal_read") {
      sets.internalReads.insert(varName);
    } else if (type == "external_write") {
      sets.externalWrites.insert(varName);
    } else if (type == "internal_write") {
      sets.internalWrites.insert(varName);
    } else if (type == "internal_shared") {
      sets.internalShared.insert(varName);
    } else if (type == "external_shared") {
      sets.externalShared.insert(varName);
    } else if (type == "shared_modified") {
      sets.sharedModified.insert(varName);
    }
  }
  sqlite3_finalize(stmt);

  query = "SELECT * FROM queued_writes WHERE funcname = ?;";
  db->prepareStatement(stmt, query, params);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string tid =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    std::string varName =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    if (sets.queuedWrites.find(tid) == sets.queuedWrites.end()) {
      sets.queuedWrites.insert({tid, {varName}});
    } else {
      sets.queuedWrites[tid].insert(varName);
    }
  }
  sqlite3_finalize(stmt);

  query = "SELECT * FROM finished_threads WHERE funcname = ?;";
  db->prepareStatement(stmt, query, params);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string varName =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    sets.finishedThreads.insert(varName);
  }
  sqlite3_finalize(stmt);

  query = "SELECT * FROM active_threads WHERE funcname = ?;";
  db->prepareStatement(stmt, query, params);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string varName =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    std::string tid =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    if (sets.activeThreads.find(varName) == sets.activeThreads.end()) {
      sets.activeThreads.insert({varName, {tid}});
    } else {
      sets.activeThreads[varName].insert(tid);
    }
  }
  sqlite3_finalize(stmt);

  return sets;
}

EraserSets *FunctionEraserSets::getEraserSets(std::string funcName) {
  if (funcName == currFunc) {
    return &currFuncSets;
  }
  if (functionSets.find(funcName) != functionSets.end()) {
    return &functionSets[funcName];
  }
  functionSets.insert({funcName, extractSetsFromDb(funcName)});
  return &functionSets[funcName];
}

void FunctionEraserSets::updateCurrEraserSets(EraserSets &sets) {
  if (currFuncSetsStarted) {
    combineSets(currFuncSets, sets);
  } else {
    currFuncSetsStarted = true;
    currFuncSets = sets;
  }
}

void FunctionEraserSets::saveCurrEraserSets() {
  functionSets[currFunc] = currFuncSets;
  bool alreadyInDb = checkCurrFuncInDb();
  if (!alreadyInDb) {
    insertCurrSetsIntoDb();
  } else {
    EraserSets originalSets = extractSetsFromDb(currFunc);
    if (originalSets != currFuncSets) {
      deleteCurrFuncFromDb();
      insertCurrSetsIntoDb();
    }
  }
}

void FunctionEraserSets::startNewFunction(std::string funcName) {
  currFunc = funcName;
  currFuncSets = EraserSets::defaultValue;
  currFuncSetsStarted = false;
}

// TODO - CALL ME AT SOME POINT!!!
void FunctionEraserSets::markFunctionEraserSetsAsOld() {
  std::string query =
      "UPDATE function_eraser_sets SET locks_changed = 0 AND vars_changed = 0 "
      "WHERE locks_changed = 1 OR vars_changed = 1;";

  sqlite3_stmt *stmt;
  db->prepareStatement(stmt, query);
  db->runStatement(stmt);
}