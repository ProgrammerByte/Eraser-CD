#include "call_graph.h"

CallGraph::CallGraph(Database *db) : db(db){};

void CallGraph::addNode(std::string funcName) {
  std::string query =
      "INSERT OR IGNORE INTO nodes_table (funcname) VALUES (?);";

  sqlite3_stmt *stmt;
  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);
}

void CallGraph::addEdge(std::string caller, std::string callee, bool onThread) {
  if (caller != callee) {
    std::string query = "INSERT OR IGNORE INTO adjacency_matrix (funcname1, "
                        "funcname2, on_thread) "
                        "VALUES (?, ?, ?);";

    sqlite3_stmt *stmt;
    std::vector<std::string> params = {caller, callee,
                                       db->createBoolean(onThread)};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  } else {
    std::string query =
        "UPDATE nodes_table SET recursive = 1 WHERE funcname = ?;";

    sqlite3_stmt *stmt;
    std::vector<std::string> params = {caller};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);
  }
}

void CallGraph::markNodes(std::vector<std::string> &startNodes,
                          bool reverse = false) {
  std::vector<std::string> q = {};

  for (const auto &node : startNodes) {
    q.push_back(node);
  }

  sqlite3_stmt *stmt;

  while (!q.empty()) {
    std::string query = "UPDATE nodes_table SET marked = 1, recently_changed = "
                        "1 WHERE funcname IN " +
                        db->createTupleList(q) + ";";
    db->prepareStatement(stmt, query, q);
    db->runStatement(stmt);

    if (reverse) {
      query =
          "WITH neighbors AS ("
          " SELECT funcname1 AS funcname, COUNT(*) AS cnt FROM "
          "adjacency_matrix "
          " JOIN nodes_table ON nodes_table.funcname = "
          " adjacency_matrix.funcname1 WHERE adjacency_matrix.funcname2 IN " +
          db->createTupleList(q) +
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
          " adjacency_matrix.funcname2 WHERE adjacency_matrix.on_thread = 0 "
          " AND adjacency_matrix.funcname1 IN " +
          db->createTupleList(q) +
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

    db->prepareStatement(stmt, query, q);
    db->runStatement(stmt);

    query =
        "SELECT funcname FROM nodes_table WHERE marked = 0 AND indegree > 0;";

    db->prepareStatement(stmt, query);

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
  db->prepareStatement(stmt, query);
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
          db->createTupleList(q) + ";";
  db->prepareStatement(stmt, query, q);
  db->runStatement(stmt);

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
          db->createTupleList(q) +
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
          " adjacency_matrix.funcname2 WHERE adjacency_matrix.on_thread = 0 "
          "AND adjacency_matrix.funcname1 IN " +
          db->createTupleList(q) +
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

    db->prepareStatement(stmt, query, q);
    db->runStatement(stmt);

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

std::vector<std::string> CallGraph::functionVariableLocksetsOrdering(
    std::vector<std::string> functions) {
  // TODO - INCLUDE PARENTS OF FUNCTIONS WITH CHANGED DELTA LOCKSETS HERE
  // LATER!!!
  markNodes(functions);
  return traverseGraph();
}

// bottom up only!!!
bool CallGraph::shouldVisitNode(std::string funcName) {
  sqlite3_stmt *stmt;
  std::string query =
      "SELECT 1 FROM nodes_table WHERE funcname = ? AND recently_changed = 1;";

  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);
  bool result = sqlite3_step(stmt) == SQLITE_ROW;
  sqlite3_finalize(stmt);
  return result;
}