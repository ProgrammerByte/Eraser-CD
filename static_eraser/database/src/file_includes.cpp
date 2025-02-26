#include "file_includes.h"

FileIncludes::FileIncludes(Database *db) : db(db){};

void FileIncludes::clearIncludes(std::string fileName) {
  sqlite3_stmt *stmt;
  std::string query = "DELETE FROM file_includes WHERE filename = ?;";

  std::vector<std::string> params = {fileName};
  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);
}

void FileIncludes::addInclude(std::string fileName, std::string includedFile) {
  sqlite3_stmt *stmt;
  std::string query = "INSERT OR IGNORE INTO file_includes "
                      "(filename, included_file) VALUES (?, ?);";

  std::vector<std::string> params = {fileName, includedFile};
  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);
}

std::set<std::string> FileIncludes::getChildren(std::string fileName) {
  sqlite3_stmt *stmt;
  std::string query =
      "SELECT filename FROM file_includes WHERE included_file = ?;";

  std::vector<std::string> params = {fileName};
  db->prepareStatement(stmt, query, params);

  std::set<std::string> includes;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    includes.insert(db->getStringFromStatement(stmt, 0));
  }

  sqlite3_finalize(stmt);
  return includes;
}