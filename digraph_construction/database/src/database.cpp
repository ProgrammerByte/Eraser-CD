#include "database.h"

void Database::prepareStatement(sqlite3_stmt *&stmt, std::string query,
                                std::vector<std::string> &params) {
  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "Error preparing statement: " << sqlite3_errmsg(db)
              << std::endl;
    return;
  }
  for (int i = 0; i < params.size(); i++) {
    if (sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_STATIC) !=
        SQLITE_OK) {
      std::cerr << "Error binding parameter: " << sqlite3_errmsg(db) << i + 1
                << std::endl;
      return;
    }
  }
}

void Database::prepareStatement(sqlite3_stmt *&stmt, std::string query) {
  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "Error preparing statement: " << sqlite3_errmsg(db)
              << std::endl;
  }
}

void Database::runStatement(sqlite3_stmt *stmt) {
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::cerr << "Error running statement: " << sqlite3_errmsg(db) << std::endl;
  }
  sqlite3_finalize(stmt);
}

// TODO - DELETING / CREATING DATABASE FOR PROTOTYPE ONLY!!!
void Database::deleteDatabase() {
  if (std::remove(dbName.c_str()) == 0) {
    std::cout << "Database file \"" << dbName << "\" deleted successfully"
              << std::endl;
  } else {
    std::cout << "Database file \"" << dbName
              << "\" does not exist or could not be deleted." << std::endl;
  }
}

void Database::createTable(std::string query, std::string tableName) {
  if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "Error creating " << tableName << ":\n" << errMsg << std::endl;
    sqlite3_free(errMsg);
  }
}

void Database::createTables() {
  createTable(R"(
    CREATE TABLE nodes_table (
      funcname TEXT PRIMARY KEY,
      recursive BOOLEAN DEFAULT FALSE,
      indegree INTEGER DEFAULT 0,
      marked BOOLEAN DEFAULT FALSE,
      recently_changed BOOLEAN DEFAULT TRUE,
      filename TEXT,
      stale BOOLEAN DEFAULT FALSE
    );
  )",
              "nodes_table");

  createTable(R"(
    CREATE TABLE adjacency_matrix (
      funcname1 TEXT,
      funcname2 TEXT,
      on_thread BOOLEAN DEFAULT FALSE,
      FOREIGN KEY (funcname1) REFERENCES nodes(funcname) ON DELETE CASCADE,
      FOREIGN KEY (funcname2) REFERENCES nodes(funcname) ON DELETE CASCADE,
      UNIQUE(funcname1, funcname2, on_thread)
    );
  )",
              "adjacency_matrix");

  createTable(R"(
    CREATE TABLE function_eraser_sets (
      funcname TEXT,
      locks_changed BOOLEAN DEFAULT TRUE,
      vars_changed BOOLEAN DEFAULT TRUE,
      FOREIGN KEY (funcname) REFERENCES nodes(funcname) ON DELETE CASCADE
    );
  )",
              "function_eraser_sets");

  createTable(R"(
    CREATE TABLE function_locks (
      funcname TEXT,
      varname TEXT,
      type TEXT CHECK(type IN ('lock', 'unlock')),
      FOREIGN KEY (funcname) REFERENCES function_eraser_sets(funcname) ON DELETE CASCADE,
      UNIQUE(funcname, varname)
    );
  )",
              "function_locks");

  createTable(R"(
    CREATE TABLE function_recursive_unlocks (
      funcname TEXT,
      varname TEXT,
      FOREIGN KEY (funcname) REFERENCES function_eraser_sets(funcname) ON DELETE CASCADE,
      UNIQUE(funcname, varname)
    );
  )",
              "function_recursive_unlocks");

  createTable(R"(
    CREATE TABLE function_vars (
      funcname TEXT,
      varname TEXT,
      type TEXT CHECK(type IN (
        'external_read',
        'internal_read',
        'external_write',
        'internal_write',
        'external_shared',
        'internal_shared',
        'shared_modified'
      )),
      FOREIGN KEY (funcname) REFERENCES function_eraser_sets(funcname) ON DELETE CASCADE,
      UNIQUE(funcname, varname, type)
    );
  )",
              "function_vars");

  createTable(R"(
    CREATE TABLE queued_writes (
      funcname TEXT,
      tid TEXT,
      varname TEXT,
      FOREIGN KEY (funcname) REFERENCES function_eraser_sets(funcname) ON DELETE CASCADE,
      UNIQUE(funcname, tid, varname)
    );
  )",
              "queued_writes");

  createTable(R"(
    CREATE TABLE finished_threads (
      funcname TEXT,
      varname TEXT,
      FOREIGN KEY (funcname) REFERENCES function_eraser_sets(funcname) ON DELETE CASCADE,
      UNIQUE(funcname, varname)
    );
  )",
              "finished_threads");

  createTable(R"(
    CREATE TABLE active_threads (
      funcname TEXT,
      varname TEXT,
      tid TEXT,
      FOREIGN KEY (funcname) REFERENCES function_eraser_sets(funcname) ON DELETE CASCADE,
      UNIQUE(funcname, varname, tid)
    );
  )",
              "active_threads");

  // TODO - THIS NEEDS TO BE USED FOR EVICTING CACHE IN TOP DOWN STAGE I.E.
  // GARBAGE COLLECTION VIA REFERENCE COUNTING
  createTable(R"(
    CREATE TABLE function_variable_locksets (
      id INTEGER PRIMARY KEY,
      funcname TEXT,
      testname TEXT,
      recently_changed BOOLEAN DEFAULT TRUE,
      FOREIGN KEY (funcname) REFERENCES nodes(funcname) ON DELETE CASCADE,
      UNIQUE(funcname, testname)
    );
  )",
              "function_variable_locksets");

  createTable(R"(
    CREATE TABLE function_variable_locksets_combined_inputs (
      function_variable_locksets_id INTEGER,
      lock TEXT,
      FOREIGN KEY (function_variable_locksets_id) REFERENCES function_variable_locksets(id) ON DELETE CASCADE,
      UNIQUE(function_variable_locksets_id, lock)
    );
  )",
              "function_variable_locksets");

  createTable(R"(
     CREATE TABLE function_variable_locksets_callers (
        id INTEGER PRIMARY KEY,
        function_variable_locksets_id INTEGER,
        caller TEXT,
        FOREIGN KEY (function_variable_locksets_id) REFERENCES
        function_variable_locksets(id) ON DELETE CASCADE,
        UNIQUE(function_variable_locksets_id, caller)
     );
   )",
              "function_variable_locksets_callers");

  createTable(R"(
     CREATE TABLE function_variable_locksets_callers_locks (
        function_variable_locksets_callers_id INTEGER,
        lock TEXT,
        FOREIGN KEY (function_variable_locksets_callers_id) REFERENCES
        function_variable_locksets_callers(id) ON DELETE CASCADE,
        UNIQUE(function_variable_locksets_callers_id, lock)
     );
   )",
              "function_variable_locksets_callers_locks");

  createTable(R"(
    CREATE TABLE function_variable_locksets_outputs (
      function_variable_locksets_id INTEGER,
      varname TEXT,
      lock TEXT,
      FOREIGN KEY (function_variable_locksets_id) REFERENCES function_variable_locksets(id) ON DELETE CASCADE,
      UNIQUE(function_variable_locksets_id, varname, lock)
    );
  )",
              "function_variable_locksets_outputs");

  createTable(R"(
    CREATE TABLE function_variable_direct_accesses (
      funcname TEXT,
      varname TEXT,
      type TEXT CHECK(type IN ('read', 'write')),
      FOREIGN KEY (funcname) REFERENCES nodes(funcname) ON DELETE CASCADE,
      UNIQUE(funcname, varname, type)
    );
  )",
              "function_variable_direct_accesses");

  createTable(R"(
    CREATE TABLE function_variable_cumulative_locksets (
      id INTEGER PRIMARY KEY,
      funcname TEXT,
      testname TEXT,
      recently_changed BOOLEAN DEFAULT TRUE,
      FOREIGN KEY (funcname) REFERENCES nodes(funcname) ON DELETE CASCADE,
      UNIQUE(funcname, testname)
    );
  )",
              "function_variable_cumulative_locksets");

  createTable(R"(
    CREATE TABLE function_variable_cumulative_locksets_outputs (
      function_variable_cumulative_locksets_id INTEGER,
      varname TEXT,
      lock TEXT,
      FOREIGN KEY (function_variable_cumulative_locksets_id) REFERENCES function_variable_cumulative_locksets(id) ON DELETE CASCADE,
      UNIQUE(function_variable_cumulative_locksets_id, varname, lock)
    );
  )",
              "function_variable_cumulative_locksets_outputs");

  createTable(R"(
    CREATE TABLE function_variable_cumulative_accesses (
      funcname TEXT,
      varname TEXT,
      type TEXT CHECK(type IN ('read', 'write')),
      FOREIGN KEY (funcname) REFERENCES nodes(funcname) ON DELETE CASCADE
      UNIQUE(funcname, varname, type)
    );
  )",
              "function_variable_cumulative_accesses");
}

Database::Database() {
  deleteDatabase();

  if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
    std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
    return;
  }

  createTables();
}

std::string Database::createTupleList(std::vector<std::string> &nodes) {
  std::string tupleList = "(";
  for (size_t i = 0; i < nodes.size(); ++i) {
    tupleList += "?";
    if (i != nodes.size() - 1) {
      tupleList += ", ";
    }
  }
  tupleList += ")";
  return tupleList;
}

std::string Database::createBoolean(bool value) {
  if (value) {
    return "1";
  }
  return "0";
}

bool Database::retrieveBoolean(std::string value) { return value == "1"; }

Database::~Database() { sqlite3_close(db); }