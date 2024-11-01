#include <clang-c/Index.h>
#include <iostream>
#include <unordered_map>
#include <vector>
using namespace std;

struct VariableInfo {
  int scopeDepth; // Depth of the scope (0 for global, etc.)
  bool isStatic;  // True if the variable is static, false otherwise
};

enum BranchType { NONE, IF, ELSE_IF, ELSE, WHILE };

static vector<unordered_map<string, VariableInfo>> scopeStack(0);
static int inFunc = 0;
static int scopeDepth = 0;
static bool ignoreNextCompound = false;

// Utility to convert CXString to ostream easily
ostream &operator<<(ostream &stream, const CXString &str) {
  stream << clang_getCString(str);
  clang_disposeString(str);
  return stream;
}

std::string getCursorFilename(CXCursor cursor) {
  CXSourceLocation location = clang_getCursorLocation(cursor);

  CXFile file;
  unsigned line, column, offset; // TODO - USE LINE NUMBER FROM HERE IN MESSAGES
  clang_getFileLocation(location, &file, &line, &column, &offset);

  if (file == nullptr) {
    return "";
  }

  CXString filename = clang_getFileName(file);
  std::string result = clang_getCString(filename);
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
    std::string token = clang_getCString(tokenSpelling);

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
  CXString funcName = clang_getCursorSpelling(cursor);
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
                  cout << "pointer to " << clang_getCursorSpelling(inner)
                       << ", ";
                }
                return CXChildVisit_Break;
              },
              nullptr);
          return CXChildVisit_Continue;
        }

        return CXChildVisit_Continue;
      },
      nullptr);
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
  // If not found then assume global and defined elsewhere
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

  if (cursorKind == CXCursor_BinaryOperator ||
      cursorKind == CXCursor_UnaryOperator) {
    CXCursor lhsCursor = clang_getCursorReferenced(cursor);
    if (clang_getCursorKind(lhsCursor) == CXCursor_DeclRefExpr) {
      CXString lhsVarName = clang_getCursorSpelling(lhsCursor);
      cout << "Variable write: '" << lhsVarName << "'" << endl;
      clang_disposeString(lhsVarName);
    }
  }

  string action = isWriteLhs ? "Written to " : "Read from ";
  cout << action << staticType << variableType << varName << location << endl;
}

BranchType getBranchType(CXCursor cursor, CXCursor parent) {
  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  CXCursorKind parentKind = clang_getCursorKind(parent);
  if (cursorKind == CXCursor_IfStmt) {
    return IF;
  } else if (parentKind == CXCursor_IfStmt &&
             cursorKind == CXCursor_CompoundStmt) {
    CXCursor child = clang_getCursorReferenced(cursor);
    if (clang_getCursorKind(child) == CXCursor_IfStmt) {
      return ELSE_IF;
    }
    return ELSE;
  } else if (cursorKind == CXCursor_WhileStmt) {
    return WHILE;
  }
  return NONE;
}

CXChildVisitResult visitor(CXCursor cursor, CXCursor parent,
                           CXClientData clientData) {
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

    // TODO - start new DAG here
    CXString funcName = clang_getCursorSpelling(cursor);

  } else if (cursorKind == CXCursor_CompoundStmt) {
    if (ignoreNextCompound) {
      // maybe start DAG here instead???
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

  bool *writeLhsPtr = reinterpret_cast<bool *>(clientData);
  bool initialWriteLhs = *writeLhsPtr;

  if (cursorKind == CXCursor_VarDecl || cursorKind == CXCursor_DeclRefExpr ||
      cursorKind == CXCursor_ParmDecl) {
    classifyVariable(cursor, initialWriteLhs);
  }

  if (!initialWriteLhs && isAssignmentOperator(cursor)) {
    *writeLhsPtr = true;
  }

  BranchType branchType = getBranchType(cursor, parent);

  // TODO - WHEN VISITING CHILDREN - APPEND NODES FROM THERE ONTO CURRENT BRANCH
  // NODE IF APPLICABLE
  clang_visitChildren(cursor, visitor, writeLhsPtr);
  if (branchType != NONE) {
    // TODO - HANDLE ME!!!
  }
  if (cursorKind == CXCursor_CompoundStmt) {
    scopeStack.pop_back();
    scopeDepth -= 1;
  }
  if (cursorKind == CXCursor_FunctionDecl) {
    ignoreNextCompound = false;
    inFunc = 0;
    // MAYBE END DAG HERE INSTEAD???
  }
  if (initialWriteLhs) {
    *writeLhsPtr = false;
  }

  return CXChildVisit_Continue;
}

int main() {
  scopeStack.push_back(unordered_map<string, VariableInfo>());

  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit unit = clang_parseTranslationUnit(
      index, "../test_files/single_files/largest_check.c", nullptr, 0, nullptr,
      0, CXTranslationUnit_None);

  if (unit == nullptr) {
    cerr << "Unable to parse translation unit. Quitting." << endl;
    exit(-1);
  }

  CXCursor cursor = clang_getTranslationUnitCursor(unit);

  bool isLhs = 0;

  clang_visitChildren(cursor, visitor, &isLhs);

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
}