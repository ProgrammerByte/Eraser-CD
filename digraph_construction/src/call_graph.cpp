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

std::string createTupleList(std::vector<std::string> &nodes) {
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

void CallGraph::markNodes(std::vector<std::string> &startNodes,
                          bool reverse = false) {
  std::vector<std::string> q = {};

  for (const auto &node : startNodes) {
    q.push_back(node);
  }

  sqlite3_stmt *stmt;

  while (!q.empty()) {
    std::string query = "UPDATE nodes_table SET marked = 1 WHERE funcname "
                        "IN " +
                        createTupleList(q) + ";";
    prepareStatement(stmt, query, q);
    runStatement(stmt);

    if (reverse) {
      query =
          "WITH neighbors AS ("
          " SELECT funcname1 AS funcname, COUNT(*) AS cnt FROM "
          "adjacency_matrix "
          " JOIN nodes_table ON nodes_table.funcname = "
          " adjacency_matrix.funcname1 WHERE adjacency_matrix.funcname2 IN " +
          createTupleList(q) +
          " GROUP BY funcname1"
          "),"
          "result AS ("
          " SELECT neighbors.funcname FROM neighbors"
          " INNER JOIN nodes_table ON nodes_table.funcname = neighbors.funcname"
          " WHERE marked = 0"
          ")"
          "UPDATE nodes_table "
          "SET indegree = indegree + COALESCE(("
          " SELECT cnt FROM neighbors WHERE neighbors.funcname = "
          " nodes_table.funcname"
          "), 0);";
    } else {
      query =
          "WITH neighbors AS ("
          " SELECT funcname2 AS funcname, COUNT(*) AS cnt FROM "
          "adjacency_matrix "
          " JOIN nodes_table ON nodes_table.funcname = "
          " adjacency_matrix.funcname2 WHERE adjacency_matrix.funcname1 IN " +
          createTupleList(q) +
          " GROUP BY funcname2"
          "),"
          "result AS ("
          " SELECT neighbors.funcname FROM neighbors"
          " INNER JOIN nodes_table ON nodes_table.funcname = neighbors.funcname"
          " WHERE marked = 0"
          ")"
          "UPDATE nodes_table "
          "SET indegree = indegree + COALESCE(("
          " SELECT cnt FROM neighbors WHERE neighbors.funcname = "
          " nodes_table.funcname"
          "), 0);";
    }

    prepareStatement(stmt, query, q);
    runStatement(stmt);

    query =
        "SELECT funcname FROM nodes_table WHERE marked = 0 AND indegree > 0;";

    prepareStatement(stmt, query);

    q.clear();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string neighbor =
          reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
      q.push_back(neighbor);
    }
    sqlite3_finalize(stmt);
  }
}

std::vector<std::string>
CallGraph::getNextNodes(std::vector<std::string> &order) {
  sqlite3_stmt *stmt;
  std::string query =
      "SELECT funcname FROM nodes_table WHERE marked = 1 AND indegree = 0;";
  prepareStatement(stmt, query);
  std::vector<std::string> q = {};
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string node =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    order.push_back(node);
    q.push_back(node);
  }
  sqlite3_finalize(stmt);

  query = "UPDATE nodes_table SET marked = 0 WHERE funcname "
          "IN " +
          createTupleList(q) + ";";
  prepareStatement(stmt, query, q);
  runStatement(stmt);

  return q;
}

std::vector<std::string> CallGraph::traverseGraph(bool reverse = false) {
  std::vector<std::string> order = {};
  std::vector<std::string> q = getNextNodes(order);

  while (!q.empty()) {
    sqlite3_stmt *stmt;
    std::string query;
    if (reverse) {
      query =
          "WITH neighbors AS ("
          " SELECT funcname1 AS funcname, COUNT(*) AS cnt FROM "
          "adjacency_matrix "
          " JOIN nodes_table ON nodes_table.funcname = "
          " adjacency_matrix.funcname1 WHERE adjacency_matrix.funcname2 IN " +
          createTupleList(q) +
          " GROUP BY funcname1"
          "),"
          "result AS ("
          " SELECT neighbors.funcname FROM neighbors"
          " INNER JOIN nodes_table ON nodes_table.funcname = neighbors.funcname"
          " WHERE marked = 1"
          ")"
          "UPDATE nodes_table "
          "SET indegree = indegree - COALESCE(("
          " SELECT cnt FROM neighbors WHERE neighbors.funcname = "
          " nodes_table.funcname"
          "), 0);";
    } else {
      query =
          "WITH neighbors AS ("
          " SELECT funcname2 AS funcname, COUNT(*) AS cnt FROM "
          "adjacency_matrix "
          " JOIN nodes_table ON nodes_table.funcname = "
          " adjacency_matrix.funcname2 WHERE adjacency_matrix.funcname1 IN " +
          createTupleList(q) +
          " GROUP BY funcname2"
          "),"
          "result AS ("
          " SELECT neighbors.funcname FROM neighbors"
          " INNER JOIN nodes_table ON nodes_table.funcname = neighbors.funcname"
          " WHERE marked = 1"
          ")"
          "UPDATE nodes_table "
          "SET indegree = indegree - COALESCE(("
          " SELECT cnt FROM neighbors WHERE neighbors.funcname = "
          " nodes_table.funcname"
          "), 0);";
    }

    prepareStatement(stmt, query, q);
    runStatement(stmt);

    q = getNextNodes(order);
  }

  return order;
}

// TODO - DOESN'T IGNORE THREAD CREATE EDGES AS IT SHOULD ALTHOUGH MAYBE NOT A
// HUGE ISSUE IF REUSED ELSEWHERE
std::vector<std::string>
CallGraph::deltaLocksetOrdering(std::vector<std::string> functions) {
  markNodes(functions, true);
  return traverseGraph(true);
}

CallGraph::~CallGraph() { sqlite3_close(db); }