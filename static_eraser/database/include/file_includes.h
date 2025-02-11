#pragma once
#include "database.h"
#include <set>

class FileIncludes {
public:
  explicit FileIncludes(Database *db);
  virtual ~FileIncludes() = default;
  void clearIncludes(std::string fileName);
  void addInclude(std::string fileName, std::string includedFile);
  std::set<std::string> getChildren(std::string fileName);

private:
  Database *db;
};