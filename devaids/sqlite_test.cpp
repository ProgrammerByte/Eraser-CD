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

  const char *createNodesTable = R"(
    CREATE TABLE nodes_table (
      funcname TEXT PRIMARY KEY,
      indegree INTEGER DEFAULT 0,
      marked BOOLEAN DEFAULT FALSE,
      visited BOOLEAN DEFAULT FALSE
    );
  )";

  if (sqlite3_exec(db, createNodesTable, nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    std::cerr << "Error creating nodes_table: " << errMsg << std::endl;
    sqlite3_free(errMsg);
  } else {
    std::cout << "Table nodes_table created successfully." << std::endl;
  }

  const char *createAdjacencyMatrix = R"(
    CREATE TABLE adjacency_matrix (
      funcname1 TEXT,
      funcname2 TEXT,
      FOREIGN KEY (funcname1) REFERENCES nodes(funcname),
      FOREIGN KEY (funcname2) REFERENCES nodes(funcname)
    );
  )";

  if (sqlite3_exec(db, createAdjacencyMatrix, nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    std::cerr << "Error creating adjacency_matrix: " << errMsg << std::endl;
    sqlite3_free(errMsg);
  } else {
    std::cout << "Table adjacency_matrix created successfully." << std::endl;
  }
}

void insertExampleData() {
  const char *insertNodes = R"(
    INSERT INTO nodes_table (funcname)
    VALUES ('t1'), ('t2'), ('t3'), ('f1'), ('f2'), ('f3'), ('f4'), ('f5'), ('f6');
  )";

  if (sqlite3_exec(db, insertNodes, nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "Error inserting into nodes_table: " << errMsg << std::endl;
    sqlite3_free(errMsg);
  } else {
    std::cout << "Inserted into nodes_table successfully." << std::endl;
  }

  const char *insertAdjacency = R"(
    INSERT INTO adjacency_matrix (funcname1, funcname2)
    VALUES ('t1', 'f1'), ('t2', 'f2'), ('t3', 'f5'),
           ('f1', 'f3'), ('f1', 'f4'), ('f2', 'f4'),
           ('f4', 'f6'), ('f5', 'f6')
  )";

  if (sqlite3_exec(db, insertAdjacency, nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    std::cerr << "Error inserting into adjacency_matrix: " << errMsg
              << std::endl;
    sqlite3_free(errMsg);
  } else {
    std::cout << "Inserted into adjacency_matrix successfully." << std::endl;
  }
}

void createOrdering(std::vector<std::string> &startNodes) {
  std::string startNodesList = "('" + startNodes[0] + "'";
  for (size_t i = 1; i < startNodes.size(); ++i) {
    startNodesList += ", '" + startNodes[i] + "'";
  }
  startNodesList += ")";
  std::cout << "Start nodes list: " << startNodesList << std::endl;

  std::string markStartNodes = R"(
    UPDATE nodes_table
    SET marked = 1
    WHERE funcname IN )" + startNodesList +
                               ";";

  if (sqlite3_exec(db, markStartNodes.c_str(), nullptr, nullptr, &errMsg) !=
      SQLITE_OK) {
    std::cerr << "Error marking start nodes" << errMsg << std::endl;
    sqlite3_free(errMsg);
  } else {
    std::cout << "Marked start nodes successfully" << std::endl;
  }

  std::string visit = R"(
    WITH RECURSIVE visit(start, end) AS (
      SELECT funcname1, funcname2 FROM adjacency_matrix WHERE funcname1 IN
        )" + startNodesList +
                      R"(
      UNION
      SELECT am.funcname1, am.funcname2 FROM adjacency_matrix am
      JOIN visit v ON am.funcname1 = v.end
    )
    UPDATE nodes_table
    SET indegree = (
      SELECT COUNT(*)
      FROM visit
      WHERE visit.end = nodes_table.funcname
    ),
    marked = 1 WHERE funcname IN (SELECT end FROM visit);
  )";

  std::cout << "Visit query: " << visit << std::endl;

  if (sqlite3_exec(db, visit.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::cerr << "Error preparing statement: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return;
  }
}

// compile with the following:
// g++ sqlite_test.cpp -lsqlite3
int main(int argc, char *argv[]) {
  deleteDatabase();

  if (sqlite3_open("example.db", &db) != SQLITE_OK) {
    std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
    return 0;
  }

  createTables();
  insertExampleData();
  std::vector<std::string> startNodes = {"t1", "t2"};
  createOrdering(startNodes);

  sqlite3_close(db);
}