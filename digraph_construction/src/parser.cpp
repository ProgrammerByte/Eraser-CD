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
#include "unlock_node.h"
#include "write_node.h"
#include <clang-c/Index.h>
#include <iostream>
#include <unordered_map>
#include <vector>
using namespace std;

struct VariableInfo {
  int scopeDepth; // Depth of the scope (0 for global, etc.)
  bool isStatic;  // True if the variable is static, false otherwise
};

enum BranchType {
  BRANCH_NONE,
  BRANCH_IF,
  BRANCH_ELSE_IF,
  BRANCH_ELSE,
  BRANCH_WHILE
};

static unordered_map<string, StartNode *> funcMap = {};
static vector<string> functions = {};
static vector<unordered_map<string, VariableInfo>> scopeStack(0);
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
  unsigned line, column, offset; // TODO - USE LINE NUMBER FROM HERE IN MESSAGES
  clang_getFileLocation(location, &file, &line, &column, &offset);

  if (file == nullptr) {
    return "";
  }

  CXString filename = clang_getFileName(file);
  string result = clang_getCString(filename);
  clang_disposeString(filename);

  return result;
}

bool isAssignmentOperator(CXCursor cursor) {
  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  if (cursorKind != CXCursor_BinaryOperator &&
      cursorKind != CXCursor_UnaryOperator) {
    return false;
  }

  CXToken *tokens;
  unsigned int numTokens;
  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
  clang_tokenize(tu, clang_getCursorExtent(cursor), &tokens, &numTokens);

  for (unsigned int i = 0; i < numTokens; ++i) {
    CXString tokenSpelling = clang_getTokenSpelling(tu, tokens[i]);
    string token = clang_getCString(tokenSpelling);

    if (token == "=" || token == "+=" || token == "-=" || token == "*=" ||
        token == "/=" || token == "++" || token == "--") {
      clang_disposeString(tokenSpelling);
      clang_disposeTokens(tu, tokens, numTokens);
      return true;
    }

    clang_disposeString(tokenSpelling);
  }

  clang_disposeTokens(tu, tokens, numTokens);
  return false;
}

void handleFunctionCall(CXCursor cursor) {
  string funcName = clang_getCString(clang_getCursorSpelling(cursor));
  cout << "Function call to '" << funcName << "' Arguments: ";

  clang_visitChildren(
      cursor,
      [](CXCursor c, CXCursor parent, CXClientData clientData) {
        CXCursorKind cursorKind = clang_getCursorKind(c);
        if (cursorKind == CXCursor_DeclRefExpr) {
          cout << "variable " << clang_getCursorSpelling(c) << ", ";
        } else if (clang_getCursorKind(c) == CXCursor_UnaryOperator) {
          // when & is used
          clang_visitChildren(
              c,
              [](CXCursor inner, CXCursor parent, CXClientData clientData) {
                if (clang_getCursorKind(inner) == CXCursor_DeclRefExpr) {
                  string varName =
                      clang_getCString(clang_getCursorSpelling(inner));
                  string funcName = *(string *)clientData;
                  if (funcName == "pthread_mutex_lock") {
                    environment->onAdd(new LockNode(
                        varName)); // TODO - LOCK / VARIABLE IDENTIFIERS
                  } else if (funcName == "pthread_mutex_unlock") {
                    environment->onAdd(new UnlockNode(varName));
                  }
                  cout << "pointer to " << clang_getCursorSpelling(inner)
                       << ", ";
                }
                return CXChildVisit_Break;
              },
              clientData);
          return CXChildVisit_Continue;
        }

        return CXChildVisit_Continue;
      },
      &funcName);
  cout << endl;
}

void classifyVariable(CXCursor cursor, bool isWriteLhs) {
  CXString varNameObj = clang_getCursorSpelling(cursor);
  string varName = clang_getCString(varNameObj);
  clang_disposeString(varNameObj);

  CXCursorKind cursorKind = clang_getCursorKind(cursor);

  struct VariableInfo variableInfo;
  bool infoFound = false;
  bool isDeclaration =
      cursorKind == CXCursor_VarDecl || cursorKind == CXCursor_ParmDecl;

  if (isDeclaration) {
    infoFound = true;
    variableInfo.isStatic =
        clang_Cursor_getStorageClass(cursor) == CX_SC_Static;
    variableInfo.scopeDepth = scopeDepth;
    if (scopeDepth == 0 && !variableInfo.isStatic) {
      return;
    }
    scopeStack[scopeDepth].insert({varName, variableInfo});
  } else {
    for (int i = scopeDepth; i >= 0; i--) {
      auto t = scopeStack[i].find(varName);
      if (t != scopeStack[i].end()) {
        infoFound = true;
        variableInfo = t->second;
        break;
      }
    }
  }
  // TODO - MAYBE NOT NEEDED? If not found then assume global and defined
  // elsewhere
  if (!infoFound) {
    variableInfo.scopeDepth = 0;
    variableInfo.isStatic = false;
    scopeStack[0].insert({varName, variableInfo});
  }
  bool global = variableInfo.scopeDepth == 0;
  bool isStatic = variableInfo.isStatic;
  string staticType = isStatic ? "static " : "";
  string variableType = global ? "global variable " : "local variable ";
  string location = " from " + getCursorFilename(cursor) +
                    " at line X with scope depth " + to_string(scopeDepth);

  if (cursorKind == CXCursor_VarDecl) {
    bool hasChild = false;
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData clientData) {
          bool *hasChild = reinterpret_cast<bool *>(clientData);
          *hasChild = true;
          return CXChildVisit_Break;
        },
        &hasChild);
    string action = hasChild ? "Declared and written to " : "Declared ";
    cout << action << staticType << variableType << varName << location << endl;
    return;
  }

  if (cursorKind == CXCursor_ParmDecl) {
    cout << "Declared parameter " << varName << endl;
    return;
  }

  if (isWriteLhs) {
    environment->onAdd(new WriteNode(varName)); // TODO - VARIABLE INFO
  } else {
    environment->onAdd(new ReadNode(varName)); // TODO - VARIABLE INFO
  }

  string action = isWriteLhs ? "Written to " : "Read from ";
  cout << action << staticType << variableType << varName << location << endl;
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
  } else if (cursorKind == CXCursor_WhileStmt) {
    return BRANCH_WHILE;
  }
  return BRANCH_NONE;
}

struct VisitorData {
  unsigned int childIndex;
  bool *writeLhsPtr;
};

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
    scopeDepth += 1;
    scopeStack.push_back(unordered_map<string, VariableInfo>());
    inFunc = 1;
    funcName = clang_getCString(clang_getCursorSpelling(cursor));
  } else if (cursorKind == CXCursor_CompoundStmt) {
    if (ignoreNextCompound) {
      startNode = environment->startNewTree(funcName);
      ignoreNextCompound = false;
    } else {
      scopeDepth += 1;
      scopeStack.push_back(unordered_map<string, VariableInfo>());
    }
  }

  if (cursorKind == CXCursor_CallExpr) {
    handleFunctionCall(cursor);
    return CXChildVisit_Recurse;
  }

  VisitorData *visitorData = reinterpret_cast<VisitorData *>(clientData);
  unsigned int childIndex = visitorData->childIndex;
  bool *writeLhsPtr = visitorData->writeLhsPtr;

  bool initialWriteLhs = *writeLhsPtr;

  if (cursorKind == CXCursor_VarDecl || cursorKind == CXCursor_DeclRefExpr ||
      cursorKind == CXCursor_ParmDecl) {
    classifyVariable(cursor, initialWriteLhs);
  }

  if (cursorKind == CXCursor_BreakStmt) {
    environment->onAdd(new BreakNode());
  } else if (cursorKind == CXCursor_ContinueStmt) {
    environment->onAdd(new ContinueNode());
  } else if (cursorKind == CXCursor_ReturnStmt) {
    environment->onAdd(new ReturnNode());
  } else if (!initialWriteLhs && isAssignmentOperator(cursor)) {
    *writeLhsPtr = true;
  }

  BranchType branchType = getBranchType(cursor, parent, childIndex);

  if (branchType == BRANCH_IF) {
    environment->onAdd(new IfNode());
  } else if (branchType == BRANCH_ELSE_IF || branchType == BRANCH_ELSE) {
    environment->onElseAdd();
  } else if (branchType == BRANCH_WHILE) {
    environment->onAdd(new WhileNode());
  }

  // TODO - WHEN VISITING CHILDREN - APPEND NODES FROM THERE ONTO CURRENT BRANCH
  // NODE BRANCH_IF APPLICABLE
  VisitorData childData = {0, writeLhsPtr};
  clang_visitChildren(cursor, visitor, &childData);
  if (cursorKind == CXCursor_IfStmt) {
    environment->onAdd(new EndifNode());
  } else if (branchType == BRANCH_WHILE) {
    environment->onAdd(new EndwhileNode());
  }
  if (cursorKind == CXCursor_CompoundStmt) {
    scopeStack.pop_back();
    scopeDepth -= 1;
  }
  if (cursorKind == CXCursor_FunctionDecl) {
    ignoreNextCompound = false;
    inFunc = 0;
    functions.push_back(funcName);
    funcMap.insert({funcName, startNode});
    startNode = nullptr;
  }
  if (initialWriteLhs) {
    *writeLhsPtr = false;
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

  bool isLhs = 0;
  VisitorData initialData = {0, &isLhs};

  environment = new ConstructionEnvironment();
  clang_visitChildren(cursor, visitor, &initialData);
  GraphVisualizer *visualizer = new GraphVisualizer();
  visualizer->visualizeGraph(funcMap[functions[0]]);

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
}