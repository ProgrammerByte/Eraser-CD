#include "variable_locksets.h"
#include "set_operations.h"

VariableLocksets::VariableLocksets(
    CallGraph *callGraph, FunctionVariableLocksets *functionVariableLocksets) {
  this->callGraph = callGraph;
  this->functionVariableLocksets = functionVariableLocksets;
}

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
  if (funcCallLocksets.find(functionName) == funcCallLocksets.end()) {
    funcCallLocksets.insert({functionName, locks});
  } else {
    funcCallLocksets[functionName] *= locks;
  }
  functionVariableLocksets->applyDeltaLockset(locks, functionName);
  return true;
};

bool VariableLocksets::handleNode(ThreadCreateNode *node,
                                  std::set<std::string> &locks) {
  std::string varName = node->varName;
  if (node->global && varName != "") {
    variableWrite(varName, locks);
  }
  return true;
};

bool VariableLocksets::handleNode(ThreadJoinNode *node,
                                  std::set<std::string> &locks) {
  std::string varName = node->varName;
  if (node->global && varName != "") {
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
  return variableRead(node->varName, locks);
};

bool VariableLocksets::handleNode(WriteNode *node,
                                  std::set<std::string> &locks) {
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
  } else if (auto *returnNode = dynamic_cast<ReturnNode *>(node)) {
    return handleNode(returnNode, locks);
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
  functionVariableLocksets->startNewFunction(currFunc, currTest);
  forwardQueue.push(startNode);
  nodeLocks = {};
  nodeLocks.insert({startNode, {}});
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
  // functionVariableLocksets->saveCurrlockss();
}

// TODO - startNode unlocks should be used by static eraser to remove from
// current lockset before calling (recursive) functions
void VariableLocksets::updateLocksets(
    std::unordered_map<std::string, StartNode *> funcCfgs,
    std::vector<std::string> changedFunctions) {
  this->funcCfgs = funcCfgs;

  std::vector<std::string> ordering =
      callGraph->functionVariableLocksetsOrdering(changedFunctions);

  for (const std::string &funcName : ordering) {
    if (!functionVariableLocksets->shouldVisitNode(funcName)) {
      continue;
    }
    currFunc = funcName;
    FunctionInputs functionInputs =
        functionVariableLocksets->updateAndCheckCombinedInputs();
    std::unordered_map<std::string, std::set<std::string>> combinedInputs =
        functionInputs.changedTests;

    std::cout << "Function: " << funcName << std::endl;
    for (const auto &pair : combinedInputs) {
      currTest = pair.first;
      std::set<std::string> startLocks = pair.second;
      handleFunction(funcCfgs[funcName], startLocks);

      std::unordered_map<std::string, std::set<std::string>> variableLocks =
          functionVariableLocksets->getVariableLocks();

      std::cout << "Test: " << currTest << std::endl;
      for (const auto &pair : variableLocks) {
        std::string varName = pair.first;
        std::set<std::string> locks = pair.second;
        std::cout << "Variable: " << varName << std::endl;
        std::cout << "Locks: ";
        for (const std::string &lock : locks) {
          std::cout << lock << ", ";
        }
        std::cout << std::endl;
      }
    }
    std::cout << std::endl;

    /*
    std::set<std::string> *sets =
    functionVariableLocksets->getlockss(funcName); std::cout << "Function: "
    << funcName << std::endl; std::cout << "Locks: "; for (const std::string
    &lock : sets->locks) { std::cout << lock << ", ";
    }
    std::cout << std::endl;
    std::cout << "Unlocks: ";
    for (const std::string &lock : sets->unlocks) {
      std::cout << lock << ", ";
    }
    std::cout << std::endl;

    std::cout << "External Reads: ";
    for (const std::string &read : sets->externalReads) {
      std::cout << read << ", ";
    }
    std::cout << std::endl;

    std::cout << "Internal Reads: ";
    for (const std::string &read : sets->internalReads) {
      std::cout << read << ", ";
    }
    std::cout << std::endl;

    std::cout << "External Writes: ";
    for (const std::string &write : sets->externalWrites) {
      std::cout << write << ", ";
    }
    std::cout << std::endl;

    std::cout << "Internal Writes: ";
    for (const std::string &write : sets->internalWrites) {
      std::cout << write << ", ";
    }
    std::cout << std::endl;

    std::cout << "Internal Shared: ";
    for (const std::string &shared : sets->internalShared) {
      std::cout << shared << ", ";
    }
    std::cout << std::endl;

    std::cout << "External Shared: ";
    for (const std::string &shared : sets->externalShared) {
      std::cout << shared << ", ";
    }
    std::cout << std::endl;

    std::cout << "Shared Modified: ";
    for (const std::string &shared : sets->sharedModified) {
      std::cout << shared << ", ";
    }
    std::cout << std::endl;

    std::cout << "Queued Writes: ";
    for (const auto &pair : sets->queuedWrites) {
      std::cout << pair.first << ": ";
      for (const std::string &write : pair.second) {
        std::cout << write << ", ";
      }
    }
    std::cout << std::endl;

    std::cout << "Active Threads: ";
    for (const auto &pair : sets->activeThreads) {
      std::cout << pair.first << ": ";
      for (const std::string &tid : pair.second) {
        std::cout << tid << ", ";
      }
    }
    std::cout << std::endl;

    std::cout << "Finished Threads: ";
    for (const std::string &tid : sets->finishedThreads) {
      std::cout << tid << ", ";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    */
  }
}