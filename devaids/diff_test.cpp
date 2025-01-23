#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

std::string executeCommand(const std::string &command) {
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

std::vector<std::string> getChangedFiles(const std::string &repoPath,
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

int main() {
  try {
    std::string repoPath = "~/dissertation/Eraser-CD";
    std::string commitHash1 = "6cef939f0f3298e04ee615af08c738eae5cce353";
    std::string commitHash2 = "6d3f917fe1a0251458a9d5ed7c56ebb968ab3cc4";

    std::vector<std::string> changedFiles =
        getChangedFiles(repoPath, commitHash1, commitHash2);

    std::cout << "Changed files between " << commitHash1 << " and "
              << commitHash2 << ":\n";
    for (const auto &file : changedFiles) {
      std::cout << file << std::endl;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
