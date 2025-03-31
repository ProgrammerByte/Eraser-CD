#include "diff_analysis.h"
#include "set_operations.h"

DiffAnalysis::DiffAnalysis(FileIncludes *fileIncludes)
    : fileIncludes(fileIncludes){};

std::string DiffAnalysis::executeCommand(const std::string &command) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, int (*)(FILE *)> pipe(popen(command.c_str(), "r"),
                                              static_cast<int (*)(FILE *)>(pclose));
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

std::set<std::string>
DiffAnalysis::getChangedFiles(const std::string &repoPath,
                              const std::string &commitHash1,
                              const std::string &commitHash2) {
  std::string changeDirCmd = "cd " + repoPath + " && ";

  std::string gitDiffCmd =
      changeDirCmd + "git diff --name-only " + commitHash1 + " " + commitHash2;

  std::string output = executeCommand(gitDiffCmd);

  std::set<std::string> changedFiles;
  std::istringstream stream(output);
  std::string file;
  while (std::getline(stream, file)) {
    if (!file.empty() && (file.size() > 2) &&
        ((file.substr(file.size() - 2) == ".c") ||
         (file.substr(file.size() - 2) == ".h"))) {
      changedFiles.insert(file);
      changedFiles += fileIncludes->getChildren(file);
    }
  }

  return changedFiles;
}

std::set<std::string> DiffAnalysis::getAllFiles(const std::string &repoPath) {
  std::string changeDirCmd = "cd " + repoPath + " && ";
  std::string findCmd =
      changeDirCmd + "find . -type f \\( -name '*.c' -o -name '*.h' \\)";

  std::string output = executeCommand(findCmd);

  std::set<std::string> allFiles;
  std::istringstream stream(output);
  std::string file;
  while (std::getline(stream, file)) {
    if (!file.empty()) {
      allFiles.insert(repoPath + "/" + file.substr(2));
    }
  }

  return allFiles;
}