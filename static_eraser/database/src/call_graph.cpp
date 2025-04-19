#include "call_graph.h"

CallGraph::CallGraph(Database *db) : db(db){};

void CallGraph::addNode(std::string funcName, std::string fileName) {
  std::string query =
      "INSERT INTO functions_table (funcname, filename) VALUES (?, ?) "
      "ON CONFLICT(funcname) DO UPDATE SET "
      "stale = 0, recently_changed = 1, filename = excluded.filename;";

  sqlite3_stmt *stmt;
  std::vector<std::string> params = {funcName, fileName};
  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);
}

void CallGraph::addEdge(std::string caller, std::string callee, bool onThread) {
  std::string query;
  sqlite3_stmt *stmt;
  std::vector<std::string> params;

  if (caller != callee) {
    query = "INSERT OR IGNORE INTO functions_table (funcname, recently_changed) "
            "VALUES (?, 0);";
    params = {callee};
    db->prepareStatement(stmt, query, params);
    db->runStatement(stmt);

    query = "INSERT OR IGNORE INTO function_calls (caller, "
            "callee, on_thread) "
            "VALUES (?, ?, ?);";

    params = {caller, callee, db->createBoolean(onThread)};
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
    std::string query = "UPDATE functions_table SET marked = 1, recently_changed = "
                        "1 WHERE funcname IN " +
                        db->createTupleList(q) + ";";
    db->prepareStatement(stmt, query, q);
    db->runStatement(stmt);

    if (reverse) {
      query =
          "WITH neighbors AS ("
          " SELECT caller AS funcname, COUNT(*) AS cnt FROM "
          "function_calls "
          " JOIN functions_table ON functions_table.funcname = "
          " function_calls.caller WHERE function_calls.callee IN " +
          db->createTupleList(q) +
          " AND functions_table.filename IS NOT NULL"
          " GROUP BY caller"
          "),"
          "result AS ("
          " SELECT neighbors.funcname FROM neighbors"
          " INNER JOIN functions_table ON functions_table.funcname = neighbors.funcname"
          " WHERE marked = 0"
          ")"
          "UPDATE functions_table "
          "SET indegree = indegree + COALESCE(("
          " SELECT cnt FROM neighbors WHERE neighbors.funcname = "
          " functions_table.funcname"
          "), 0);";
    } else {
      query =
          "WITH neighbors AS ("
          " SELECT callee AS funcname, COUNT(*) AS cnt FROM "
          "function_calls "
          " JOIN functions_table ON functions_table.funcname = "
          " function_calls.callee WHERE "
          " function_calls.caller IN " +
          db->createTupleList(q) +
          " AND functions_table.filename IS NOT NULL"
          " GROUP BY callee"
          "),"
          "result AS ("
          " SELECT neighbors.funcname FROM neighbors"
          " INNER JOIN functions_table ON functions_table.funcname = neighbors.funcname"
          " WHERE marked = 0"
          ")"
          "UPDATE functions_table "
          "SET indegree = indegree + COALESCE(("
          " SELECT cnt FROM neighbors WHERE neighbors.funcname = "
          " functions_table.funcname"
          "), 0);";
    }

    db->prepareStatement(stmt, query, q);
    db->runStatement(stmt);

    query =
        "SELECT funcname FROM functions_table WHERE marked = 0 AND indegree > 0;";

    db->prepareStatement(stmt, query);

    q.clear();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      std::string neighbor = db->getStringFromStatement(stmt, 0);
      q.push_back(neighbor);
    }
    sqlite3_finalize(stmt);
  }
}

std::vector<std::string>
CallGraph::getNextNodes(std::vector<std::string> &order) {
  sqlite3_stmt *stmt;
  std::string query =
      "SELECT funcname FROM functions_table WHERE marked = 1 AND indegree = 0;";
  db->prepareStatement(stmt, query);
  std::vector<std::string> q = {};
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string node = db->getStringFromStatement(stmt, 0);
    order.push_back(node);
    q.push_back(node);
  }
  sqlite3_finalize(stmt);

  query = "UPDATE functions_table SET marked = 0 WHERE funcname "
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
          " SELECT caller AS funcname, COUNT(*) AS cnt FROM "
          "function_calls "
          " JOIN functions_table ON functions_table.funcname = "
          " function_calls.caller WHERE function_calls.callee IN " +
          db->createTupleList(q) +
          " AND functions_table.filename IS NOT NULL"
          " GROUP BY caller"
          "),"
          "result AS ("
          " SELECT neighbors.funcname FROM neighbors"
          " INNER JOIN functions_table ON functions_table.funcname = neighbors.funcname"
          " WHERE marked = 1"
          ")"
          "UPDATE functions_table "
          "SET indegree = indegree - COALESCE(("
          " SELECT cnt FROM neighbors WHERE neighbors.funcname = "
          " functions_table.funcname"
          "), 0);";
    } else {
      query =
          "WITH neighbors AS ("
          " SELECT callee AS funcname, COUNT(*) AS cnt FROM "
          "function_calls "
          " JOIN functions_table ON functions_table.funcname = "
          " function_calls.callee WHERE "
          " function_calls.caller IN " +
          db->createTupleList(q) +
          " AND functions_table.filename IS NOT NULL"
          " GROUP BY callee"
          "),"
          "result AS ("
          " SELECT neighbors.funcname FROM neighbors"
          " INNER JOIN functions_table ON functions_table.funcname = neighbors.funcname"
          " WHERE marked = 1"
          ")"
          "UPDATE functions_table "
          "SET indegree = indegree - COALESCE(("
          " SELECT cnt FROM neighbors WHERE neighbors.funcname = "
          " functions_table.funcname"
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
      "SELECT 1 FROM functions_table WHERE funcname = ? AND recently_changed = 1;";

  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);
  bool result = sqlite3_step(stmt) == SQLITE_ROW;
  sqlite3_finalize(stmt);
  return result;
}

void CallGraph::markNodesAsStale(std::string fileName) {
  sqlite3_stmt *stmt;
  std::string query = "UPDATE functions_table SET stale = 1 WHERE filename = ? AND "
                      "recently_changed = 0;";

  std::vector<std::string> params = {fileName};
  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);

  query = "DELETE FROM function_calls WHERE caller IN "
          "(SELECT funcname FROM functions_table WHERE filename = ?);";

  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);

  query = "DELETE FROM function_recursive_unlocks WHERE funcname IN "
          "(SELECT funcname FROM functions_table WHERE filename = ?);";

  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);

  query = "DELETE FROM function_variable_locksets_callers WHERE caller IN "
          "(SELECT funcname FROM functions_table WHERE filename = ?);";

  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);

  query =
      "DELETE FROM function_cumulative_locksets_outputs WHERE "
      "function_cumulative_locksets_id IN "
      "(SELECT id FROM function_cumulative_locksets WHERE funcname IN "
      "(SELECT funcname FROM functions_table WHERE filename = ?));";

  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);

  query = "DELETE FROM function_cumulative_accesses WHERE funcname IN "
          "(SELECT funcname FROM functions_table WHERE filename = ?);";

  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);

  query = "UPDATE function_variable_locksets SET recently_changed = 1 WHERE "
          "funcname IN "
          "(SELECT funcname FROM functions_table WHERE filename = ?);";

  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);
}

void CallGraph::deleteStaleNodes() {
  sqlite3_stmt *stmt;
  std::string query = "DELETE FROM functions_table WHERE stale = 1";

  db->prepareStatement(stmt, query);
  db->runStatement(stmt);
}

std::string CallGraph::getFilenameFromFuncname(std::string funcName) {
  sqlite3_stmt *stmt;
  std::string query = "SELECT filename FROM functions_table WHERE funcname = ?;";

  std::vector<std::string> params = {funcName};
  db->prepareStatement(stmt, query, params);
  std::string result = "";
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    result = db->getStringFromStatement(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return result;
}