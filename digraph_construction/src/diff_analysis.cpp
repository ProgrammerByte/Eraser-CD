#include "diff_analysis.h"

std::string DiffAnalysis::executeCommand(const std::string &command) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"),
                                                pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

std::vector<std::string>
DiffAnalysis::getChangedFiles(const std::string &repoPath,
                              const std::string &commitHash1,
                              const std::string &commitHash2) {
  std::string changeDirCmd = "cd " + repoPath + " && ";

  std::string gitDiffCmd =
      changeDirCmd + "git diff --name-only " + commitHash1 + " " + commitHash2;

  std::string output = executeCommand(gitDiffCmd);

  std::vector<std::string> changedFiles;
  std::istringstream stream(output);
  std::string file;
  while (std::getline(stream, file)) {
    if (!file.empty()) {
      changedFiles.push_back(file);
    }
  }

  return changedFiles;
}