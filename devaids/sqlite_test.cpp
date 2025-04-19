#include <cstdio>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>

const std::string dbName = "example.db";
sqlite3 *db;
char *errMsg = 0;

void deleteDatabase() {
  if (std::remove(dbName.c_str()) == 0) {
    std::cout << "Database file \"" << dbName << "\" deleted successfully"
              << std::endl;
  } else {
    std::cout << "Database file \"" << dbName
              << "\" does not exist or could not be deleted." << std::endl;
  }
}

void createTables() {
  std::cout << "Created database successfully" << std::endl;

  std::string query = R"(
    CREATE TABLE functions_table (
      funcname TEXT PRIMARY KEY,
      indegree INTEGER DEFAULT 0,
      marked BOOLEAN DEFAULT FALSE
    );
  )";

  if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "Error creating functions_table: " << errMsg << std::endl;
    sqlite3_free(errMsg);
  } else {
    std::cout << "Table functions_table created successfully." << std::endl;
  }

  query = R"(
    CREATE TABLE function_calls (
      caller TEXT,
      callee TEXT,
      FOREIGN KEY (caller) REFERENCES nodes(funcname),
      FOREIGN KEY (callee) REFERENCES nodes(funcname)
      UNIQUE(caller, callee)
    );
  )";

  if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "Error creating function_calls: " << errMsg << std::endl;
    sqlite3_free(errMsg);
  } else {
    std::cout << "Table function_calls created successfully." << std::endl;
  }
}

void insertExampleData() {
  std::string query = R"(
    INSERT INTO functions_table (funcname)
    VALUES ('t1'), ('t2'), ('t3'), ('f1'), ('f2'), ('f3'), ('f4'), ('f5'), ('f6'), ('f7');
  )";

  if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "Error inserting into functions_table: " << errMsg << std::endl;
    sqlite3_free(errMsg);
  } else {
    std::cout << "Inserted into functions_table successfully." << std::endl;
  }

  query = R"(
    INSERT INTO function_calls (caller, callee)
    VALUES ('t1', 'f1'), ('t2', 'f2'), ('t3', 'f5'),
           ('f1', 'f3'), ('f1', 'f4'), ('f2', 'f4'), ('f2', 'f6'),
           ('f2', 'f7'), ('f4', 'f6'), ('f5', 'f6'), ('f7', 'f6')
  )";

  if (sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "Error inserting into function_calls: " << errMsg
              << std::endl;
    sqlite3_free(errMsg);
  } else {
    std::cout << "Inserted into function_calls successfully." << std::endl;
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

void prepareStatement(sqlite3_stmt *&stmt, std::string query,
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

void prepareStatement(sqlite3_stmt *&stmt, std::string query) {
  if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "Error preparing statement: " << sqlite3_errmsg(db)
              << std::endl;
  }
}

void runStatement(sqlite3_stmt *stmt) {
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::cerr << "Error running statement: " << sqlite3_errmsg(db) << std::endl;
  }
  sqlite3_finalize(stmt);
}

void markNodes(std::vector<std::string> &startNodes, bool reverse = false) {
  std::vector<std::string> q = {};

  for (const auto &node : startNodes) {
    q.push_back(node);
  }

  sqlite3_stmt *stmt;

  while (!q.empty()) {
    std::string query = "UPDATE functions_table SET marked = 1 WHERE funcname "
                        "IN " +
                        createTupleList(q) + ";";
    prepareStatement(stmt, query, q);
    runStatement(stmt);

    if (reverse) {
      query =
          "WITH neighbors AS ("
          " SELECT caller AS funcname, COUNT(*) AS cnt FROM "
          "function_calls "
          " JOIN functions_table ON functions_table.funcname = "
          " function_calls.caller WHERE function_calls.callee IN " +
          createTupleList(q) +
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
          " function_calls.callee WHERE function_calls.caller IN " +
          createTupleList(q) +
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

    prepareStatement(stmt, query, q);
    runStatement(stmt);

    query =
        "SELECT funcname FROM functions_table WHERE marked = 0 AND indegree > 0;";

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

std::vector<std::string> getNextNodes(std::vector<std::string> &order) {
  sqlite3_stmt *stmt;
  std::string query =
      "SELECT funcname FROM functions_table WHERE marked = 1 AND indegree = 0;";
  prepareStatement(stmt, query);
  std::vector<std::string> q = {};
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::string node =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    order.push_back(node);
    q.push_back(node);
  }
  sqlite3_finalize(stmt);

  query = "UPDATE functions_table SET marked = 0 WHERE funcname "
          "IN " +
          createTupleList(q) + ";";
  prepareStatement(stmt, query, q);
  runStatement(stmt);

  return q;
}

std::vector<std::string> traverseGraph(bool reverse = false) {
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
          createTupleList(q) +
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
          " function_calls.callee WHERE function_calls.caller IN " +
          createTupleList(q) +
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

    prepareStatement(stmt, query, q);
    runStatement(stmt);

    q = getNextNodes(order);
  }

  return order;
}

// compile with the following:
// g++ sqlite_test.cpp -lsqlite3
int main(int argc, char *argv[]) {
  deleteDatabase();

  if (sqlite3_open("example.db", &db) != SQLITE_OK) {
    std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    return 0;
  }

  sqlite3_config(
      SQLITE_CONFIG_LOG,
      [](void *, int errCode, const char *msg) {
        std::cerr << "SQLite Log [" << errCode << "]: " << msg << std::endl;
      },
      nullptr);

  createTables();
  insertExampleData();
  std::vector<std::string> startNodes = {"f1", "f6"};
  markNodes(startNodes, true);
  std::vector<std::string> order = traverseGraph(true);
  std::cout << "Ordering for delta locksets:" << std::endl;
  for (int i = 0; i < order.size(); i++) {
    std::cout << order[i] << std::endl;
  }

  startNodes = {"t2"};
  markNodes(startNodes);
  order = traverseGraph();
  std::cout << "Ordering for static Eraser:" << std::endl;
  for (int i = 0; i < order.size(); i++) {
    std::cout << order[i] << std::endl;
  }

  sqlite3_close(db);
}