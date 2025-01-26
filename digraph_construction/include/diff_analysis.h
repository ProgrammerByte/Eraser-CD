#pragma once
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class DiffAnalysis {
public:
  explicit DiffAnalysis() = default;
  virtual ~DiffAnalysis() = default;

  std::vector<std::string> getChangedFiles(const std::string &repoPath,
                                           const std::string &commitHash1,
                                           const std::string &commitHash2);

  std::vector<std::string> getAllFiles(const std::string &repoPath);

private:
  std::string executeCommand(const std::string &command);
};