#pragma once
#include "break_node.h"
#include "call_graph.h"
#include "construction_environment.h"
#include "continue_node.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "file_includes.h"
#include "function_call_node.h"
#include "if_node.h"
#include "lock_node.h"
#include "read_node.h"
#include "return_node.h"
#include "start_node.h"
#include "startwhile_node.h"
#include "thread_create_node.h"
#include "thread_join_node.h"
#include "unlock_node.h"
#include "write_node.h"
#include <clang-c/Index.h>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

enum BranchType {
  BRANCH_NONE,
  BRANCH_IF,
  BRANCH_ELSE_IF,
  BRANCH_ELSE,
  BRANCH_STARTWHILE,
  BRANCH_WHILE,
  BRANCH_DO_WHILE_START,
  BRANCH_DO_WHILE_COND,
  BRANCH_FOR_START,
  BRANCH_FOR_ITERATOR,
  BRANCH_FOR
};

enum LhsType { LHS_NONE, LHS_WRITE, LHS_READ_AND_WRITE };

struct VariableInfo {
  int scopeDepth;
  int scopeNum;
  bool isStatic;
  bool isAtomic;
};

struct VisitorData {
  unsigned int childIndex;
  std::vector<GraphNode *> nodesToAdd;
  LhsType lhsType;
};

inline std::ostream &operator<<(std::ostream &stream, const CXString &str) {
  stream << clang_getCString(str);
  clang_disposeString(str);
  return stream;
}

extern std::unordered_map<std::string, StartNode *> funcCfgs;
static std::unordered_map<std::string, bool> funcMap = {};
static std::vector<std::string> functions = {};
static std::set<std::string> functionDeclarations = {};
static std::vector<std::unordered_map<std::string, VariableInfo>> scopeStack =
    {};
static std::vector<unsigned int> scopeNums = {0};
static int inFunc = 0;
static int scopeDepth = 0;
static bool ignoreNextCompound = false;
static std::string funcName = "";
static StartNode *startNode = nullptr;
static ConstructionEnvironment *environment;
static CallGraph *callGraph;

class Parser {
public:
  explicit Parser(CallGraph *callGraph, FileIncludes *fileIncludes);
  virtual ~Parser() = default;

  void parseFile(const char *fileName);
  std::vector<std::string> getFunctions();

private:
  FileIncludes *fileIncludes;
  std::string fileNameString;
};