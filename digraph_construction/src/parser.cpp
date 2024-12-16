#include "break_node.h"
#include "construction_environment.h"
#include "continue_node.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "function_call_node.h"
#include "graph_visualizer.h"
#include "if_node.h"
#include "lock_node.h"
#include "read_node.h"
#include "return_node.h"
#include "start_node.h"
#include "startwhile_node.h"
#include "unlock_node.h"
#include "write_node.h"
#include <clang-c/Index.h>
#include <iostream>
#include <unordered_map>
#include <vector>
using namespace std;

struct VariableInfo {
  int scopeDepth; // Depth of the scope (0 for global, etc.)
  int scopeNum;
  bool isStatic; // True if the variable is static, false otherwise
};

enum BranchType {
  BRANCH_NONE,
  BRANCH_IF,
  BRANCH_ELSE_IF,
  BRANCH_ELSE,
  BRANCH_STARTWHILE,
  BRANCH_WHILE
};

enum LhsType { LHS_NONE, LHS_WRITE, LHS_READ_AND_WRITE };

static unordered_map<string, StartNode *> funcCfgs = {};
static unordered_map<string, bool> funcMap = {};
static vector<string> functions = {};
static vector<unordered_map<string, VariableInfo>> scopeStack = {};
static vector<unsigned int> scopeNums = {0};
static int inFunc = 0;
static int scopeDepth = 0;
static bool ignoreNextCompound = false;
static string funcName = "";
static StartNode *startNode = nullptr;
static ConstructionEnvironment *environment;

// Utility to convert CXString to ostream easily
ostream &operator<<(ostream &stream, const CXString &str) {
  stream << clang_getCString(str);
  clang_disposeString(str);
  return stream;
}

string getCursorFilename(CXCursor cursor) {
  CXSourceLocation location = clang_getCursorLocation(cursor);

  CXFile file;
  unsigned line, column, offset;
  clang_getFileLocation(location, &file, &line, &column, &offset);

  if (file == nullptr) {
    return "";
  }

  CXString filename = clang_getFileName(file);
  string result = clang_getCString(filename);
  clang_disposeString(filename);

  return result;
}

LhsType assignmentOperatorType(CXCursor cursor) {
  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  if (cursorKind != CXCursor_BinaryOperator &&
      cursorKind != CXCursor_UnaryOperator &&
      cursorKind != CXCursor_CompoundAssignOperator) {
    return LHS_NONE;
  }

  CXToken *tokens;
  unsigned int numTokens;
  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
  clang_tokenize(tu, clang_getCursorExtent(cursor), &tokens, &numTokens);

  for (unsigned int i = 0; i < numTokens; ++i) {
    CXString tokenSpelling = clang_getTokenSpelling(tu, tokens[i]);
    string token = clang_getCString(tokenSpelling);

    if (token == "=") {
      clang_disposeString(tokenSpelling);
      clang_disposeTokens(tu, tokens, numTokens);
      return LHS_WRITE;
    } else if (token == "+=" || token == "-=" || token == "*=" ||
               token == "/=" || token == "++" || token == "--") {
      clang_disposeString(tokenSpelling);
      clang_disposeTokens(tu, tokens, numTokens);
      return LHS_READ_AND_WRITE;
    }

    clang_disposeString(tokenSpelling);
  }

  clang_disposeTokens(tu, tokens, numTokens);
  return LHS_NONE;
}

VariableInfo findVariableInfo(string varName) {
  for (int i = scopeDepth; i >= 0; i--) {
    auto t = scopeStack[i].find(varName);
    if (t != scopeStack[i].end()) {
      return t->second;
    }
  }
  struct VariableInfo variableInfo;
  variableInfo.scopeDepth = 0;
  variableInfo.scopeNum = 0;
  variableInfo.isStatic = false;
  scopeStack[0].insert({varName, variableInfo});
  return variableInfo;
}

void handleFunctionCall(CXCursor cursor, vector<GraphNode *> *nodesToAdd) {
  string funcName = clang_getCString(clang_getCursorSpelling(cursor));

  if (funcName != "pthread_mutex_lock" && funcName != "pthread_mutex_unlock") {
    auto func = funcMap.find(funcName);
    if (func != funcMap.end() && func->second) {
      string fileName = getCursorFilename(cursor);
      funcName = fileName + " " + funcName;
    }
    (*nodesToAdd).push_back(new FunctionCallNode(funcName));
    return;
  }

  clang_visitChildren(
      cursor,
      [](CXCursor c, CXCursor parent, CXClientData clientData) {
        CXCursorKind cursorKind = clang_getCursorKind(c);
        if (clang_getCursorKind(c) == CXCursor_UnaryOperator) {
          // when & is used
          clang_visitChildren(
              c,
              [](CXCursor inner, CXCursor parent, CXClientData clientData) {
                if (clang_getCursorKind(inner) == CXCursor_DeclRefExpr) {
                  string varName =
                      clang_getCString(clang_getCursorSpelling(inner));
                  struct VariableInfo variableInfo = findVariableInfo(varName);
                  if (variableInfo.isStatic) {
                    string fileName = getCursorFilename(inner);
                    varName = fileName + " " +
                              to_string(variableInfo.scopeDepth) + " " +
                              to_string(variableInfo.scopeNum) + " " + varName;
                  }
                  string funcName = *(string *)clientData;
                  if (funcName == "pthread_mutex_lock") {
                    environment->onAdd(new LockNode(varName));
                  } else if (funcName == "pthread_mutex_unlock") {
                    environment->onAdd(new UnlockNode(varName));
                  }
                }
                return CXChildVisit_Break;
              },
              clientData);
          return CXChildVisit_Continue;
        }

        return CXChildVisit_Continue;
      },
      &funcName);
}

void classifyVariable(CXCursor cursor, LhsType lhsType,
                      vector<GraphNode *> *nodesToAdd) {
  CXString varNameObj = clang_getCursorSpelling(cursor);
  string varName = clang_getCString(varNameObj);
  clang_disposeString(varNameObj);

  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  CXType cursorType = clang_getCursorType(cursor);

  if (cursorKind == CXCursor_DeclRefExpr &&
      cursorType.kind == CXType_FunctionProto) {
    return;
  }

  struct VariableInfo variableInfo;
  bool infoFound = false;
  bool isDeclaration =
      cursorKind == CXCursor_VarDecl || cursorKind == CXCursor_ParmDecl;

  if (isDeclaration) {
    infoFound = true;
    variableInfo.isStatic =
        clang_Cursor_getStorageClass(cursor) == CX_SC_Static;
    variableInfo.scopeDepth = scopeDepth;
    variableInfo.scopeNum = scopeNums[scopeDepth];
    scopeStack[scopeDepth].insert({varName, variableInfo});
    return;
  } else {
    variableInfo = findVariableInfo(varName);
  }
  if (!variableInfo.isStatic && variableInfo.scopeDepth > 0) {
    return;
  }
  CXString typeSpelling = clang_getTypeSpelling(cursorType);
  if (string(clang_getCString(typeSpelling)) == "pthread_mutex_t") {
    clang_disposeString(typeSpelling);
    return;
  }
  clang_disposeString(typeSpelling);

  bool global = variableInfo.scopeDepth == 0;

  if (variableInfo.isStatic) {
    string fileName = getCursorFilename(cursor);
    varName = fileName + " " + to_string(variableInfo.scopeDepth) + " " +
              to_string(variableInfo.scopeNum) + " " + varName;
  }

  if (lhsType == LHS_WRITE) {
    (*nodesToAdd).push_back(new WriteNode(varName));
  } else if (lhsType == LHS_READ_AND_WRITE) {
    (*nodesToAdd).push_back(new ReadNode(varName));
    (*nodesToAdd).push_back(new WriteNode(varName));
  } else {
    environment->onAdd(new ReadNode(varName));
  }
}

BranchType getBranchType(CXCursor cursor, CXCursor parent,
                         unsigned int childIndex) {
  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  CXCursorKind parentKind = clang_getCursorKind(parent);
  if (parentKind == CXCursor_IfStmt) {
    if (cursorKind == CXCursor_CompoundStmt) {
      if (childIndex == 1) {
        return BRANCH_IF;
      } else if (childIndex == 2) {
        return BRANCH_ELSE;
      }
    } else if (cursorKind == CXCursor_IfStmt) {
      return BRANCH_ELSE_IF;
    }
  } else if (parentKind == CXCursor_WhileStmt) {
    if (childIndex == 0) {
      return BRANCH_STARTWHILE;
    } else if (childIndex == 1) {
      return BRANCH_WHILE;
    }
  }
  return BRANCH_NONE;
}

struct VisitorData {
  unsigned int childIndex;
  LhsType *lhsTypePtr;
  vector<GraphNode *> nodesToAdd;
};

void onNewScope() {
  scopeDepth += 1;
  scopeStack.push_back(unordered_map<string, VariableInfo>());
  if (scopeDepth >= scopeNums.size()) {
    scopeNums.push_back(0);
  } else {
    scopeNums[scopeDepth] += 1;
  }
}

CXChildVisitResult visitor(CXCursor cursor, CXCursor parent,
                           CXClientData clientData) {
  // TODO - REMOVE THIS IF STATEMENT IN A BIT!!!
  if (clang_Location_isFromMainFile(clang_getCursorLocation(cursor)) == 0) {
    return CXChildVisit_Continue;
  }
  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  CXCursorKind parentKind =
      clang_getCursorKind(clang_getCursorSemanticParent(cursor));

  if (cursorKind == CXCursor_FunctionDecl) {
    ignoreNextCompound = true;
    onNewScope();
    inFunc = 1;
    funcName = clang_getCString(clang_getCursorSpelling(cursor));
    bool isStatic = clang_Cursor_getStorageClass(cursor) == CX_SC_Static;
    funcMap.insert({funcName, isStatic});
    if (isStatic) {
      string fileName = getCursorFilename(cursor);
      funcName = fileName + " " + funcName;
    }
  } else if (cursorKind == CXCursor_CompoundStmt) {
    if (ignoreNextCompound) {
      startNode = environment->startNewTree(funcName);
      ignoreNextCompound = false;
    } else {
      onNewScope();
    }
  }

  VisitorData *visitorData = reinterpret_cast<VisitorData *>(clientData);
  unsigned int childIndex = visitorData->childIndex;
  LhsType *lhsTypePtr = visitorData->lhsTypePtr;
  LhsType initialLhsType = *lhsTypePtr;

  VisitorData childData = {0, lhsTypePtr, {}};
  if (cursorKind == CXCursor_CallExpr) {
    handleFunctionCall(cursor, &childData.nodesToAdd);
  } else if (cursorKind == CXCursor_VarDecl ||
             cursorKind == CXCursor_DeclRefExpr ||
             cursorKind == CXCursor_ParmDecl) {
    classifyVariable(cursor, initialLhsType, &visitorData->nodesToAdd);
  } else if (cursorKind == CXCursor_BreakStmt) {
    environment->onAdd(new BreakNode());
  } else if (cursorKind == CXCursor_ContinueStmt) {
    environment->onAdd(new ContinueNode());
  } else if (initialLhsType == LHS_NONE) {
    LhsType cursorLhsType = assignmentOperatorType(cursor);
    if (cursorLhsType != LHS_NONE) {
      *lhsTypePtr = cursorLhsType;
    }
  }

  BranchType branchType = getBranchType(cursor, parent, childIndex);

  if (branchType == BRANCH_IF) {
    environment->onAdd(new IfNode());
  } else if (branchType == BRANCH_ELSE_IF || branchType == BRANCH_ELSE) {
    environment->onElseAdd();
  } else if (branchType == BRANCH_STARTWHILE) {
    environment->onAdd(new StartwhileNode());
  } else if (branchType == BRANCH_WHILE) {
    environment->onAdd(new WhileNode());
  }

  clang_visitChildren(cursor, visitor, &childData);
  for (int i = 0; i < childData.nodesToAdd.size(); i++) {
    environment->onAdd(childData.nodesToAdd[i]);
  }
  if (cursorKind == CXCursor_IfStmt) {
    environment->onAdd(new EndifNode());
  } else if (branchType == BRANCH_WHILE) {
    environment->onAdd(new ContinueNode());
    environment->onAdd(new EndwhileNode());
  } else if (cursorKind == CXCursor_ReturnStmt) {
    environment->onAdd(new ReturnNode());
  }
  if (cursorKind == CXCursor_CompoundStmt) {
    scopeStack.pop_back();
    scopeDepth -= 1;
  }
  if (cursorKind == CXCursor_FunctionDecl) {
    ignoreNextCompound = false;
    inFunc = 0;
    environment->onAdd(new ReturnNode());
    functions.push_back(funcName);
    funcCfgs.insert({funcName, startNode});
    startNode = nullptr;
  }
  if (initialLhsType) {
    *lhsTypePtr = LHS_NONE;
  }

  visitorData->childIndex += 1;
  return CXChildVisit_Continue;
}

int main() {
  scopeStack.push_back(unordered_map<string, VariableInfo>());

  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit unit = clang_parseTranslationUnit(
      index, "test_files/single_files/largest_check.c", nullptr, 0, nullptr, 0,
      CXTranslationUnit_None);

  if (unit == nullptr) {
    cerr << "Unable to parse translation unit. Quitting." << endl;
    exit(-1);
  }

  CXCursor cursor = clang_getTranslationUnitCursor(unit);

  LhsType lhsType = LHS_NONE;
  VisitorData initialData = {0, &lhsType, {}};

  environment = new ConstructionEnvironment();
  clang_visitChildren(cursor, visitor, &initialData);
  GraphVisualizer *visualizer = new GraphVisualizer();
  visualizer->visualizeGraph(funcCfgs[functions[1]]);

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
}