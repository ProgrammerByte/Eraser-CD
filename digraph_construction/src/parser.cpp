#include "break_node.h"
#include "call_graph.h"
#include "construction_environment.h"
#include "continue_node.h"
#include "delta_lockset.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "function_call_node.h"
#include "graph_visualizer.h"
#include "if_node.h"
#include "lock_node.h"
#include "over_approximated_static_eraser.h"
#include "read_node.h"
#include "return_node.h"
#include "start_node.h"
#include "startwhile_node.h"
#include "thread_create_node.h"
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
  BRANCH_WHILE,
  BRANCH_DO_WHILE_START,
  BRANCH_DO_WHILE_COND,
  BRANCH_FOR_START,
  BRANCH_FOR_ITERATOR,
  BRANCH_FOR
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
static CallGraph *callGraph;

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

LhsType assignmentOperatorType(CXCursor parent, unsigned int childIndex) {
  if (childIndex != 0) {
    return LHS_NONE;
  }

  CXCursorKind parentKind = clang_getCursorKind(parent);

  CXSourceRange range = clang_getCursorExtent(parent);
  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(parent);

  CXToken *tokens = nullptr;
  unsigned int numTokens = 0;
  clang_tokenize(tu, range, &tokens, &numTokens);

  LhsType result = LHS_NONE;

  for (unsigned int i = 0; i < numTokens; ++i) {
    CXString tokenSpelling = clang_getTokenSpelling(tu, tokens[i]);
    string token = clang_getCString(tokenSpelling);

    if (parentKind == CXCursor_BinaryOperator && token == "=") {
      result = LHS_WRITE;
    } else if (parentKind == CXCursor_UnaryOperator &&
               (token == "++" || token == "--")) {
      result = LHS_READ_AND_WRITE;
    } else if (parentKind == CXCursor_CompoundAssignOperator) {
      result = LHS_READ_AND_WRITE;
    }

    clang_disposeString(tokenSpelling);
  }

  clang_disposeTokens(tu, tokens, numTokens);

  return result;
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

CXCursor getFirstChild(CXCursor cursor) {
  CXCursor firstChild = clang_getNullCursor();

  clang_visitChildren(
      cursor,
      [](CXCursor c, CXCursor parent, CXClientData clientData) {
        CXCursor *firstChildPtr = reinterpret_cast<CXCursor *>(clientData);

        *firstChildPtr = c;
        return CXChildVisit_Break;
      },
      &firstChild);

  return firstChild;
}

string getThirdArg(CXCursor cursor) {
  struct ArgClientData {
    int argNum;
    CXCursor *argCursor;
  };

  struct ArgClientData clientData = {0, NULL};

  clang_visitChildren(
      cursor,
      [](CXCursor c, CXCursor parent, CXClientData clientData) {
        struct ArgClientData *argClientData =
            reinterpret_cast<struct ArgClientData *>(clientData);

        argClientData->argNum++;

        if (argClientData->argNum == 4) {
          argClientData->argCursor = &c;
          return CXChildVisit_Break;
        }

        return CXChildVisit_Continue;
      },
      &clientData);

  if (clientData.argNum >= 4) {
    CXCursor argCursor = *clientData.argCursor;

    while (clang_getCursorKind(argCursor) == CXCursor_UnexposedExpr) {
      argCursor = getFirstChild(argCursor);
    }
    if (clang_getCursorKind(argCursor) == CXCursor_DeclRefExpr) {
      CXString argSpelling = clang_getCursorSpelling(argCursor);
      string result = clang_getCString(argSpelling);
      clang_disposeString(argSpelling);
      return result;
    }
  }
  return "";
}

string getFuncName(CXCursor cursor, string funcName) {
  auto func = funcMap.find(funcName);
  if (func != funcMap.end() && func->second) {
    string fileName = getCursorFilename(cursor);
    funcName = fileName + " " + funcName;
  }
  return funcName;
}

void handleFunctionCall(CXCursor cursor, vector<GraphNode *> *nodesToAdd) {
  string caller = funcName;
  string funcName = clang_getCString(clang_getCursorSpelling(cursor));

  if (funcName == "pthread_create") {
    string called = getThirdArg(cursor);
    if (called != "") {
      string funcName = getFuncName(cursor, called);
      environment->onAdd(new ThreadCreateNode(funcName));
      // TODO - MAYBE CHANGE EDGE TYPE HERE?? ONLY AN OPTIMISATION FOR DELTA
      // LOCKSETS
      callGraph->addEdge(caller, funcName);
    }
  } else if (funcName == "pthread_mutex_lock" ||
             funcName == "pthread_mutex_unlock") {
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
                    struct VariableInfo variableInfo =
                        findVariableInfo(varName);
                    if (variableInfo.isStatic) {
                      string fileName = getCursorFilename(inner);
                      varName = fileName + " " +
                                to_string(variableInfo.scopeDepth) + " " +
                                to_string(variableInfo.scopeNum) + " " +
                                varName;
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
  } else if (funcName != "pthread_join") {
    funcName = getFuncName(cursor, funcName);
    (*nodesToAdd).push_back(new FunctionCallNode(funcName));
    callGraph->addEdge(caller, funcName);
  }
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
  if (parentKind == CXCursor_IfStmt ||
      parentKind == CXCursor_ConditionalOperator) {
    if (childIndex == 1) {
      return BRANCH_IF;
    } else if (childIndex == 2 && cursorKind == CXCursor_IfStmt) {
      return BRANCH_ELSE_IF;
    } else if (childIndex == 2) {
      return BRANCH_ELSE;
    }
  } else if (parentKind == CXCursor_WhileStmt) {
    if (childIndex == 0) {
      return BRANCH_STARTWHILE;
    } else if (childIndex == 1) {
      return BRANCH_WHILE;
    }
  } else if (parentKind == CXCursor_ForStmt) {
    if (childIndex == 1) {
      return BRANCH_FOR_START;
    } else if (childIndex == 2) {
      return BRANCH_FOR_ITERATOR;
    } else if (childIndex == 3) {
      return BRANCH_FOR;
    }
  } else if (parentKind == CXCursor_DoStmt) {
    if (childIndex == 0) {
      return BRANCH_DO_WHILE_START;
    } else if (childIndex == 1) {
      return BRANCH_DO_WHILE_COND;
    }
  }
  return BRANCH_NONE;
}

struct VisitorData {
  unsigned int childIndex;
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

  VisitorData childData = {0, {}};
  if (cursorKind == CXCursor_CallExpr) {
    handleFunctionCall(cursor, &childData.nodesToAdd);
  } else if (cursorKind == CXCursor_VarDecl ||
             cursorKind == CXCursor_DeclRefExpr ||
             cursorKind == CXCursor_ParmDecl) {
    LhsType lhsType = assignmentOperatorType(parent, childIndex);
    classifyVariable(cursor, lhsType, &visitorData->nodesToAdd);
  } else if (cursorKind == CXCursor_BreakStmt) {
    environment->onAdd(new BreakNode());
  } else if (cursorKind == CXCursor_ContinueStmt) {
    environment->onAdd(new ContinueNode());
  }

  BranchType branchType = getBranchType(cursor, parent, childIndex);
  WhileNode *forNodeLoop = nullptr;

  if (branchType == BRANCH_IF) {
    environment->onAdd(new IfNode());
  } else if (branchType == BRANCH_ELSE_IF || branchType == BRANCH_ELSE) {
    environment->onElseAdd();
  } else if (branchType == BRANCH_STARTWHILE) {
    environment->onAdd(new StartwhileNode());
  } else if (branchType == BRANCH_WHILE) {
    environment->onAdd(new WhileNode());
  } else if (branchType == BRANCH_DO_WHILE_START ||
             branchType == BRANCH_FOR_START) {
    StartwhileNode *startwhileNode = new StartwhileNode();
    startwhileNode->continueReturn = nullptr;
    startwhileNode->isDoWhile = branchType == BRANCH_DO_WHILE_START;
    environment->onAdd(startwhileNode);
  } else if (branchType == BRANCH_DO_WHILE_COND) {
    environment->onAdd(new ContinueReturnNode());
  } else if (branchType == BRANCH_FOR_ITERATOR) {
    forNodeLoop = new WhileNode();
    environment->onAdd(forNodeLoop);
    environment->onAdd(new ContinueReturnNode());
  }

  clang_visitChildren(cursor, visitor, &childData);
  for (int i = 0; i < childData.nodesToAdd.size(); i++) {
    environment->onAdd(childData.nodesToAdd[i]);
  }
  if (cursorKind == CXCursor_IfStmt ||
      cursorKind == CXCursor_ConditionalOperator) {
    environment->onAdd(new EndifNode());
  } else if (branchType == BRANCH_WHILE) {
    environment->onAdd(new ContinueNode());
    environment->onAdd(new EndwhileNode());
  } else if (cursorKind == CXCursor_ReturnStmt) {
    environment->onAdd(new ReturnNode());
  } else if (branchType == BRANCH_DO_WHILE_START) {
    environment->onAdd(new ContinueNode());
  } else if (branchType == BRANCH_DO_WHILE_COND) {
    WhileNode *whileNode = new WhileNode();
    whileNode->isDoWhile = true;
    environment->onAdd(whileNode);
    environment->onAdd(new EndwhileNode());
  } else if (branchType == BRANCH_FOR_ITERATOR) {
    environment->goBackToStartWhile();
    environment->currNode = forNodeLoop;
  } else if (branchType == BRANCH_FOR) {
    environment->onAdd(new ContinueNode());
    environment->onAdd(new EndwhileNode());
  }
  if (cursorKind == CXCursor_CompoundStmt) {
    scopeStack.pop_back();
    scopeDepth -= 1;
  }
  if (cursorKind == CXCursor_FunctionDecl) {
    if (ignoreNextCompound) {
      scopeStack.pop_back();
      scopeDepth -= 1;
    }
    ignoreNextCompound = false;
    inFunc = 0;
    if (startNode != nullptr) {
      environment->onAdd(new ReturnNode());
      functions.push_back(funcName);
      funcCfgs.insert({funcName, startNode});
      // TODO - FLAG AS RECURSIVE
      callGraph->addNode(funcName);
      startNode = nullptr;
    }
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

  VisitorData initialData = {0, {}};

  environment = new ConstructionEnvironment();
  callGraph = new CallGraph();
  clang_visitChildren(cursor, visitor, &initialData);
  GraphVisualizer *visualizer = new GraphVisualizer();
  visualizer->visualizeGraph(funcCfgs[functions[1]]);

  OverApproximatedStaticEraser *staticEraser =
      new OverApproximatedStaticEraser();

  DeltaLockset *deltaLockset = new DeltaLockset(callGraph);

  // staticEraser->testFunction(funcCfgs, "main");

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
}