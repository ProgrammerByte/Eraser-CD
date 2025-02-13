#include "variable_locksets.h"
#include "debug_tools.h"
#include "set_operations.h"

VariableLocksets::VariableLocksets(
    CallGraph *callGraph, Parser *parser,
    FunctionVariableLocksets *functionVariableLocksets)
    : callGraph(callGraph), parser(parser),
      functionVariableLocksets(functionVariableLocksets) {}

bool VariableLocksets::variableRead(std::string varName,
                                    std::set<std::string> &locks) {
  if (variableLocksets.find(varName) == variableLocksets.end()) {
    variableLocksets.insert({varName, locks});
  } else {
    variableLocksets[varName] *= locks;
  }
  return true;
}

bool VariableLocksets::variableWrite(std::string varName,
                                     std::set<std::string> &locks) {
  if (variableLocksets.find(varName) == variableLocksets.end()) {
    variableLocksets.insert({varName, locks});
  } else {
    variableLocksets[varName] *= locks;
  }
  return true;
}

bool VariableLocksets::handleNode(FunctionCallNode *node,
                                  std::set<std::string> &locks) {
  // TODO - MARK CURRENT NODES IN FOR QUEUE FOR CALLEE!!!
  std::string functionName = node->functionName;
  if (functionName != currFunc) {
    if (funcCallLocksets.find(functionName) == funcCallLocksets.end()) {
      funcCallLocksets.insert({functionName, locks});
    } else {
      funcCallLocksets[functionName] *= locks;
    }
  }
  functionVariableLocksets->applyDeltaLockset(locks, functionName);
  return true;
};

bool VariableLocksets::handleNode(ThreadCreateNode *node,
                                  std::set<std::string> &locks) {
  std::string varName = node->varName;
  std::string functionName = node->functionName;
  if (node->global && varName != "" && !node->eraserIgnoreOn) {
    variableWrite(varName, locks);
  }
  if (funcCallLocksets.find(functionName) == funcCallLocksets.end()) {
    funcCallLocksets.insert({functionName, {}});
  } else {
    funcCallLocksets[functionName] = {};
  }
  return true;
};

bool VariableLocksets::handleNode(ThreadJoinNode *node,
                                  std::set<std::string> &locks) {
  std::string varName = node->varName;
  if (node->global && varName != "" && !node->eraserIgnoreOn) {
    variableRead(varName, locks);
  }
  return true;
};

bool VariableLocksets::handleNode(LockNode *node,
                                  std::set<std::string> &locks) {
  locks.insert(node->varName);
  return true;
};

bool VariableLocksets::handleNode(UnlockNode *node,
                                  std::set<std::string> &locks) {
  locks.erase(node->varName);
  return true;
};

bool VariableLocksets::handleNode(ReadNode *node,
                                  std::set<std::string> &locks) {
  if (node->eraserIgnoreOn) {
    return true;
  }
  return variableRead(node->varName, locks);
};

bool VariableLocksets::handleNode(WriteNode *node,
                                  std::set<std::string> &locks) {
  if (node->eraserIgnoreOn) {
    return true;
  }
  return variableWrite(node->varName, locks);
};

bool VariableLocksets::handleNode(GraphNode *node,
                                  std::set<std::string> &locks) {
  if (auto *functionNode = dynamic_cast<FunctionCallNode *>(node)) {
    return handleNode(functionNode, locks);
  } else if (auto *threadCreateNode = dynamic_cast<ThreadCreateNode *>(node)) {
    return handleNode(threadCreateNode, locks);
  } else if (auto *threadJoinNode = dynamic_cast<ThreadJoinNode *>(node)) {
    return handleNode(threadJoinNode, locks);
  } else if (auto *lockNode = dynamic_cast<LockNode *>(node)) {
    return handleNode(lockNode, locks);
  } else if (auto *unlockNode = dynamic_cast<UnlockNode *>(node)) {
    return handleNode(unlockNode, locks);
  } else if (auto *readNode = dynamic_cast<ReadNode *>(node)) {
    return handleNode(readNode, locks);
  } else if (auto *writeNode = dynamic_cast<WriteNode *>(node)) {
    return handleNode(writeNode, locks);
  }
  return true;
};

void VariableLocksets::addNodeToQueue(GraphNode *startNode,
                                      GraphNode *nextNode) {
  if (startNode->id < nextNode->id) {
    forwardQueue.push(nextNode);
  } else {
    backwardQueue.push_back(nextNode);
  }
}

void VariableLocksets::handleFunction(GraphNode *startNode,
                                      std::set<std::string> &startLocks) {
  variableLocksets = {};
  funcCallLocksets = {};
  forwardQueue.push(startNode);
  nodeLocks = {};
  nodeLocks.insert(
      {startNode,
       startLocks - functionVariableLocksets->getFunctionRecursiveUnlocks()});
  int lastId = -1;

  while (!forwardQueue.empty() || !backwardQueue.empty()) {
    if (forwardQueue.empty()) {
      for (GraphNode *node : backwardQueue) {
        forwardQueue.push(node);
      }
      backwardQueue.clear();
    }

    GraphNode *node = forwardQueue.top();
    std::set<std::string> locks = nodeLocks[node];
    forwardQueue.pop();
    if (node->id == lastId) {
      continue;
    }
    lastId = node->id;

    std::vector<GraphNode *> nextNodes = node->getNextNodes();
    for (GraphNode *nextNode : nextNodes) {
      std::set<std::string> nextLocks = locks;

      if (handleNode(nextNode, nextLocks)) {
        if (nodeLocks.find(nextNode) == nodeLocks.end()) {
          nodeLocks.insert({nextNode, nextLocks});
          addNodeToQueue(node, nextNode);
        } else {
          nextLocks *= nodeLocks[nextNode];

          if (nextLocks != nodeLocks[nextNode]) {
            nodeLocks[nextNode] = nextLocks;
            addNodeToQueue(node, nextNode);
          }
        }
      }
    }
  }
  functionVariableLocksets->addFuncCallLocksets(funcCallLocksets);
  functionVariableLocksets->addVariableLocksets(variableLocksets);
}

void VariableLocksets::updateLocksets() {

  std::vector<std::string> functions =
      functionVariableLocksets->getFunctionsForTesting();

  std::vector<std::string> ordering =
      callGraph->functionVariableLocksetsOrdering(functions);

  for (const std::string &funcName : ordering) {
    if (!functionVariableLocksets->shouldVisitNode(funcName)) {
      debugCout << "VL SKIPPING " << funcName << std::endl;
      continue;
    }
    debugCout << "VL looking at: " << funcName << std::endl;
    currFunc = funcName;
    functionVariableLocksets->startNewFunction(currFunc);
    FunctionInputs functionInputs =
        functionVariableLocksets->updateAndCheckCombinedInputs();
    std::unordered_map<std::string, std::set<std::string>> combinedInputs =
        functionInputs.changedTests;
    // TODO - use functionInputs.reachableTests at some point!!! - revise
    // this!!!

    debugCout << "Function: " << funcName << std::endl;
    for (const auto &pair : combinedInputs) {
      currTest = pair.first;
      functionVariableLocksets->startNewTest(currTest);
      std::set<std::string> startLocks = pair.second;
      if (funcCfgs.find(funcName) == funcCfgs.end()) {
        std::string fileName = callGraph->getFilenameFromFuncname(funcName);
        parser->parseFile(fileName.c_str());
      }
      handleFunction(funcCfgs[funcName], startLocks);

      std::unordered_map<std::string, std::set<std::string>> variableLocks =
          functionVariableLocksets->getVariableLocks();

      debugCout << "Test: " << currTest << std::endl;
      for (const auto &pair : variableLocks) {
        std::string varName = pair.first;
        std::set<std::string> locks = pair.second;
        debugCout << "Variable: " << varName << std::endl;
        debugCout << "Locks: ";
        for (const std::string &lock : locks) {
          debugCout << lock << ", ";
        }
        debugCout << std::endl;
      }
    }
    debugCout << std::endl;
  }
}