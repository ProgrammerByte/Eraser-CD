#include "eraser_settings.h"

EraserSettings::EraserSettings(Database *db) : db(db){};

std::string EraserSettings::getAndUpdatePrevHash(std::string commitHash) {
  sqlite3_stmt *stmt;
  std::string query = "SELECT prev_hash FROM eraser_settings LIMIT 1;";

  db->prepareStatement(stmt, query);
  std::string prevHash = "";
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    prevHash = db->getStringFromStatement(stmt, 0);
  }
  sqlite3_finalize(stmt);

  query = "DELETE FROM eraser_settings";
  db->prepareStatement(stmt, query);
  db->runStatement(stmt);

  query = "INSERT INTO eraser_settings (prev_hash) VALUES (?);";
  std::vector<std::string> params = {commitHash};
  db->prepareStatement(stmt, query, params);
  db->runStatement(stmt);

  return prevHash;
}