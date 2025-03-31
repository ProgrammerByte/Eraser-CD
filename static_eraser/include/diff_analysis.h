#pragma once
#include "file_includes.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <array>

class DiffAnalysis {
public:
  explicit DiffAnalysis(FileIncludes *fileIncludes);
  virtual ~DiffAnalysis() = default;

  std::set<std::string> getChangedFiles(const std::string &repoPath,
                                        const std::string &commitHash1,
                                        const std::string &commitHash2);

  std::set<std::string> getAllFiles(const std::string &repoPath);

private:
  FileIncludes *fileIncludes;
  std::string executeCommand(const std::string &command);
};