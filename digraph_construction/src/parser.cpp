#include "parser.h"

std::unordered_map<std::string, StartNode *> funcCfgs;

Parser::Parser(CallGraph *callGraphPtr) {
  funcCfgs = {};
  callGraph = callGraphPtr;
  environment = new ConstructionEnvironment();
}

std::string getCursorFilename(CXCursor cursor) {
  CXSourceLocation location = clang_getCursorLocation(cursor);

  CXFile file;
  unsigned line, column, offset;
  clang_getFileLocation(location, &file, &line, &column, &offset);

  if (file == nullptr) {
    return "";
  }

  CXString filename = clang_getFileName(file);
  std::string result = clang_getCString(filename);
  clang_disposeString(filename);

  return result;
}

LhsType assignmentOperatorType(CXCursor parent) {
  CXCursorKind parentKind = clang_getCursorKind(parent);

  CXSourceRange range = clang_getCursorExtent(parent);
  CXTranslationUnit tu = clang_Cursor_getTranslationUnit(parent);

  CXToken *tokens = nullptr;
  unsigned int numTokens = 0;
  clang_tokenize(tu, range, &tokens, &numTokens);

  LhsType result = LHS_NONE;

  for (unsigned int i = 0; i < numTokens; ++i) {
    CXString tokenSpelling = clang_getTokenSpelling(tu, tokens[i]);
    std::string token = clang_getCString(tokenSpelling);

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

VariableInfo findVariableInfo(std::string varName) {
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

bool isSharedVar(struct VariableInfo variableInfo) {
  return variableInfo.isStatic || variableInfo.scopeDepth == 0;
}

bool isSharedVar(std::string varName) {
  return isSharedVar(findVariableInfo(varName));
}

std::string getVariableName(std::string varName, CXCursor cursor,
                            struct VariableInfo variableInfo) {
  if (variableInfo.isStatic || variableInfo.scopeDepth > 0) {
    std::string fileName = getCursorFilename(cursor);
    varName = fileName + " " + std::to_string(variableInfo.scopeDepth) + " " +
              std::to_string(variableInfo.scopeNum) + " " + varName;
  }
  return varName;
}

std::string getVariableName(std::string varName, CXCursor cursor) {
  return getVariableName(varName, cursor, findVariableInfo(varName));
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

std::string getNthArg(CXCursor cursor, int targetArg, bool isPtr = false) {
  struct ArgClientData {
    int targetArg;
    bool isPtr;
    int argNum;
    CXCursor *argCursor;
  };

  struct ArgClientData clientData = {targetArg, isPtr, 0, NULL};

  clang_visitChildren(
      cursor,
      [](CXCursor c, CXCursor parent, CXClientData clientData) {
        struct ArgClientData *argClientData =
            reinterpret_cast<struct ArgClientData *>(clientData);

        if (argClientData->argNum == argClientData->targetArg) {
          argClientData->argCursor = &c;
          if (argClientData->isPtr &&
              clang_getCursorKind(c) == CXCursor_UnaryOperator) {
            argClientData->isPtr = false;
            return CXChildVisit_Recurse;
          }
          return CXChildVisit_Break;
        }

        argClientData->argNum++;

        return CXChildVisit_Continue;
      },
      &clientData);

  if (clientData.argNum >= targetArg && !clientData.isPtr) {
    CXCursor argCursor = *clientData.argCursor;

    while (clang_getCursorKind(argCursor) == CXCursor_UnexposedExpr) {
      argCursor = getFirstChild(argCursor);
    }
    if (clang_getCursorKind(argCursor) == CXCursor_DeclRefExpr) {
      CXString argSpelling = clang_getCursorSpelling(argCursor);
      std::string result = clang_getCString(argSpelling);
      clang_disposeString(argSpelling);
      return result;
    }
  }
  return "";
}

std::string getFuncName(CXCursor cursor, std::string funcName) {
  auto func = funcMap.find(funcName);
  if (func != funcMap.end() && func->second) {
    std::string fileName = getCursorFilename(cursor);
    funcName = fileName + " " + funcName;
  }
  return funcName;
}

void handleFunctionCall(CXCursor cursor, std::vector<GraphNode *> *nodesToAdd) {
  std::string caller = funcName;
  std::string funcName = clang_getCString(clang_getCursorSpelling(cursor));

  if (funcName == "pthread_create") {
    std::string called = getNthArg(cursor, 3);
    if (called != "") {
      std::string spelling = getNthArg(cursor, 1, true);
      VariableInfo variableInfo = findVariableInfo(spelling);
      std::string varName = getVariableName(spelling, cursor, variableInfo);
      std::string funcName = getFuncName(cursor, called);
      bool global = isSharedVar(variableInfo);
      environment->onAdd(new ThreadCreateNode(funcName, varName, global));
      if (global && varName != "") {
        environment->onAdd(new WriteNode(varName));
      }
      // TODO - MAYBE CHANGE EDGE TYPE HERE?? ONLY AN OPTIMISATION FOR DELTA
      // LOCKSETS
      callGraph->addEdge(caller, funcName, true);
    }
  } else if (funcName == "pthread_join") {
    std::string spelling = getNthArg(cursor, 1);
    VariableInfo variableInfo = findVariableInfo(spelling);
    std::string varName = getVariableName(spelling, cursor, variableInfo);
    bool global = isSharedVar(variableInfo);
    if (varName != "") {
      environment->onAdd(new ThreadJoinNode(varName, global));
    }
    // TODO - USE SECOND ARG FOR RETURN VALUE, MAKE WRITE NODE AS NEEDED
    // however can be generalised somewhat i.e. if passing ptr to func then
    // treat as write. Okay for this algorithm
  } else if (funcName == "pthread_mutex_lock" ||
             funcName == "pthread_mutex_unlock") {
    std::string spelling = getNthArg(cursor, 1, true);
    VariableInfo variableInfo = findVariableInfo(spelling);
    if (isSharedVar(variableInfo)) {
      std::string varName = getVariableName(spelling, cursor, variableInfo);
      if (funcName == "pthread_mutex_lock") {
        environment->onAdd(new LockNode(varName));
      } else if (funcName == "pthread_mutex_unlock") {
        environment->onAdd(new UnlockNode(varName));
      }
    }
  } else {
    funcName = getFuncName(cursor, funcName);
    (*nodesToAdd).push_back(new FunctionCallNode(funcName));
    callGraph->addEdge(caller, funcName, false);
  }
}

void classifyVariable(CXCursor cursor, LhsType lhsType,
                      std::vector<GraphNode *> *nodesToAdd) {
  CXString varNameObj = clang_getCursorSpelling(cursor);
  std::string varName = clang_getCString(varNameObj);
  clang_disposeString(varNameObj);

  CXCursorKind cursorKind = clang_getCursorKind(cursor);
  CXType cursorType = clang_getCursorType(cursor);

  if (cursorKind == CXCursor_DeclRefExpr &&
      cursorType.kind == CXType_FunctionProto) {
    return;
  }

  struct VariableInfo variableInfo;
  bool isDeclaration =
      cursorKind == CXCursor_VarDecl || cursorKind == CXCursor_ParmDecl;

  if (isDeclaration) {
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
  if (std::string(clang_getCString(typeSpelling)) == "pthread_mutex_t") {
    clang_disposeString(typeSpelling);
    return;
  }
  clang_disposeString(typeSpelling);

  bool global = variableInfo.scopeDepth == 0;

  if (variableInfo.isStatic) {
    std::string fileName = getCursorFilename(cursor);
    varName = fileName + " " + std::to_string(variableInfo.scopeDepth) + " " +
              std::to_string(variableInfo.scopeNum) + " " + varName;
  }

  if (functionDeclarations.find(varName) == functionDeclarations.end()) {
    if (lhsType == LHS_WRITE) {
      (*nodesToAdd).push_back(new WriteNode(varName));
    } else if (lhsType == LHS_READ_AND_WRITE) {
      (*nodesToAdd).push_back(new ReadNode(varName));
      (*nodesToAdd).push_back(new WriteNode(varName));
    } else {
      environment->onAdd(new ReadNode(varName));
    }
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

void onNewScope() {
  scopeDepth += 1;
  scopeStack.push_back(std::unordered_map<std::string, VariableInfo>());
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
      std::string fileName = getCursorFilename(cursor);
      funcName = fileName + " " + funcName;
    }
    functionDeclarations.insert(funcName);
  } else if (cursorKind == CXCursor_CompoundStmt) {
    if (ignoreNextCompound) {
      startNode = environment->startNewTree(funcName);
      callGraph->addNode(funcName, getCursorFilename(cursor));
      ignoreNextCompound = false;
    } else {
      onNewScope();
    }
  }

  VisitorData *visitorData = reinterpret_cast<VisitorData *>(clientData);
  unsigned int childIndex = visitorData->childIndex;
  LhsType lhsType = visitorData->lhsType;
  LhsType nextLhsType = lhsType;
  if (lhsType == LHS_NONE) {
    nextLhsType = assignmentOperatorType(cursor);
  }

  VisitorData childData = {0, {}, nextLhsType};
  std::vector<GraphNode *> nodesToAddAfterChildren = {};
  if (cursorKind == CXCursor_CallExpr) {
    handleFunctionCall(cursor, &childData.nodesToAdd);
  } else if (cursorKind == CXCursor_VarDecl ||
             cursorKind == CXCursor_DeclRefExpr ||
             cursorKind == CXCursor_ParmDecl) {
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
  if (lhsType == LHS_NONE) {
    for (int i = 0; i < childData.nodesToAdd.size(); i++) {
      environment->onAdd(childData.nodesToAdd[i]);
    }
  } else {
    for (int i = 0; i < childData.nodesToAdd.size(); i++) {
      visitorData->nodesToAdd.push_back(childData.nodesToAdd[i]);
    }
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
      startNode = nullptr;
    }
  }

  visitorData->childIndex += 1;
  visitorData->lhsType = LHS_NONE;
  return CXChildVisit_Continue;
}

void Parser::parseFile(const char *fileName) {
  funcMap = {};
  functionDeclarations = {};
  scopeStack.clear();
  scopeNums = {0};
  inFunc = 0;
  scopeDepth = 0;
  ignoreNextCompound = false;
  funcName = "";
  startNode = nullptr;

  scopeStack.push_back(std::unordered_map<std::string, VariableInfo>());

  CXIndex index = clang_createIndex(0, 0);
  CXTranslationUnit unit = clang_parseTranslationUnit(
      index, fileName, nullptr, 0, nullptr, 0, CXTranslationUnit_None);

  if (unit == nullptr) {
    std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
    exit(-1);
  }

  CXCursor cursor = clang_getTranslationUnitCursor(unit);

  VisitorData initialData = {0, {}, LHS_NONE};

  clang_visitChildren(cursor, visitor, &initialData);

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
}

std::vector<std::string> Parser::getFunctions() { return functions; }