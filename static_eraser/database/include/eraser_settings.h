#pragma once
#include "database.h"

class EraserSettings {
public:
  explicit EraserSettings(Database *db);
  virtual ~EraserSettings() = default;
  std::string getAndUpdatePrevHash(std::string commitHash);

private:
  Database *db;
};