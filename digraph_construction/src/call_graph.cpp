#include "call_graph.h"

void CallGraph::prepareStatement(sqlite3_stmt *&stmt, std::string query,
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

void CallGraph::prepareStatement(sqlite3_stmt *&stmt, std::string query) {
  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "Error preparing statement: " << sqlite3_errmsg(db)
              << std::endl;
  }
}

void CallGraph::runStatement(sqlite3_stmt *stmt) {
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::cerr << "Error running statement: " << sqlite3_errmsg(db) << std::endl;
  }
  sqlite3_finalize(stmt);
}

// TODO - DELETING / CREATING DATABASE FOR PROTOTYPE ONLY!!!
void CallGraph::deleteDatabase() {
  if (std::remove(dbName.c_str()) == 0) {
    std::cout << "Database file \"" << dbName << "\" deleted successfully"
              << std::endl;
  } else {
    std::cout << "Database file \"" << dbName
              << "\" does not exist or could not be deleted." << std::endl;
  }
}

void CallGraph::createTable(std::string query, std::string tableName) {
  if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "Error creating " << tableName << ":\n" << errMsg << std::endl;
    sqlite3_free(errMsg);
  }
}

void CallGraph::createTables() {
  std::string query = R"(
    CREATE TABLE nodes_table (
      funcname TEXT PRIMARY KEY,
      recursive BOOLEAN DEFAULT FALSE,
      indegree INTEGER DEFAULT 0,
      marked BOOLEAN DEFAULT FALSE
    );
  )";

  createTable(query, "nodes_table");

  query = R"(
    CREATE TABLE adjacency_matrix (
      funcname1 TEXT,
      funcname2 TEXT,
      FOREIGN KEY (funcname1) REFERENCES nodes(funcname),
      FOREIGN KEY (funcname2) REFERENCES nodes(funcname),
      UNIQUE(funcname1, funcname2)
    );
  )";

  createTable(query, "adjacency_matrix");
}

CallGraph::CallGraph() {
  deleteDatabase();

  if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
    std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
    return;
  }

  createTables();
}

void CallGraph::addNode(std::string funcName) {
  std::string query =
      "INSERT OR IGNORE INTO nodes_table (funcname) VALUES (?);";

  sqlite3_stmt *stmt;
  std::vector<std::string> params = {funcName};
  prepareStatement(stmt, query, params);
  runStatement(stmt);
}

void CallGraph::addEdge(std::string caller, std::string callee) {
  if (caller != callee) {
    std::string query =
        "INSERT OR IGNORE INTO adjacency_matrix (funcname1, funcname2) "
        "VALUES (?, ?);";

    sqlite3_stmt *stmt;
    std::vector<std::string> params = {caller, callee};
    prepareStatement(stmt, query, params);
    runStatement(stmt);
  } else {
    std::string query =
        "UPDATE nodes_table SET recursive = 1 WHERE funcname = ?;";

    sqlite3_stmt *stmt;
    std::vector<std::string> params = {caller};
    prepareStatement(stmt, query, params);
    runStatement(stmt);
  }
}

CallGraph::~CallGraph() { sqlite3_close(db); }