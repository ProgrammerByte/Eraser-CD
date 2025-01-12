#include "delta_lockset.h"
#include "set_operations.h"

DeltaLockset::DeltaLockset(CallGraph *callGraph) {
  this->callGraph = callGraph;
}

bool DeltaLockset::recursiveFunctionCall(std::string functionName,
                                         EraserSets &sets,
                                         bool fromThread = false) {
  if (functionName == currFunc) {
    GraphNode *startNode = funcCfgs[currFunc];
    EraserSets nextSets = nodeSets[startNode];
    if (fromThread) {
      // nothing yet
    } else {
      nextSets.locks *= sets.locks;
      nextSets.unlocks += sets.unlocks;
    }
    if (nextSets != nodeSets[startNode] || !recursive) {
      nodeSets[startNode] = nextSets;
      backwardQueue.push_back(startNode);
    }
    if (!recursive) {
      return true;
    }
  }
  return false;
}

bool DeltaLockset::handleNode(FunctionCallNode *node, EraserSets &sets) {
  std::string functionName = node->functionName;
  if (recursiveFunctionCall(functionName, sets)) {
    return false;
  }
  if (functionSets.find(functionName) != functionSets.end()) {
    sets.locks -= functionSets[functionName].unlocks;
    sets.locks += functionSets[functionName].locks;
    sets.unlocks -= functionSets[functionName].locks;
    sets.unlocks += functionSets[functionName].unlocks;
  }
  return true;
};

bool DeltaLockset::handleNode(ThreadCreateNode *node, EraserSets &sets) {
  if (node->global) {
    // sharedVariableAccessed(node->varName, sets);
  }
  std::string functionName = node->functionName;
  if (recursiveFunctionCall(node->functionName, sets, true)) {
    return false;
  }
  return true;
};

bool DeltaLockset::handleNode(ThreadJoinNode *node, EraserSets &sets) {
  // TODO
  return true;
};

bool DeltaLockset::handleNode(LockNode *node, EraserSets &sets) {
  sets.locks.insert(node->varName);
  sets.unlocks.erase(node->varName);
  return true;
};

bool DeltaLockset::handleNode(UnlockNode *node, EraserSets &sets) {
  sets.locks.erase(node->varName);
  sets.unlocks.insert(node->varName);
  return true;
};

bool DeltaLockset::handleNode(ReadNode *node, EraserSets &sets) {
  return true;
};

bool DeltaLockset::handleNode(WriteNode *node, EraserSets &sets) {
  return true;
};

bool DeltaLockset::handleNode(ReturnNode *node, EraserSets &sets) {
  if (functionSets.find(currFunc) == functionSets.end()) {
    functionSets.insert({currFunc, sets});
  } else {
    functionSets[currFunc].locks *= sets.locks;
    functionSets[currFunc].unlocks += sets.unlocks;
  }
  return true;
}

bool DeltaLockset::handleNode(GraphNode *node, EraserSets &sets) {
  if (auto *functionNode = dynamic_cast<FunctionCallNode *>(node)) {
    return handleNode(functionNode, sets);
  } else if (auto *threadCreateNode = dynamic_cast<ThreadCreateNode *>(node)) {
    return handleNode(threadCreateNode, sets);
  } else if (auto *threadJoinNode = dynamic_cast<ThreadJoinNode *>(node)) {
    return handleNode(threadJoinNode, sets);
  } else if (auto *lockNode = dynamic_cast<LockNode *>(node)) {
    return handleNode(lockNode, sets);
  } else if (auto *unlockNode = dynamic_cast<UnlockNode *>(node)) {
    return handleNode(unlockNode, sets);
  } else if (auto *readNode = dynamic_cast<ReadNode *>(node)) {
    return handleNode(readNode, sets);
  } else if (auto *writeNode = dynamic_cast<WriteNode *>(node)) {
    return handleNode(writeNode, sets);
  } else if (auto *returnNode = dynamic_cast<ReturnNode *>(node)) {
    return handleNode(returnNode, sets);
  }
  return true;
};

void DeltaLockset::addNodeToQueue(GraphNode *startNode, GraphNode *nextNode) {
  if (startNode->id < nextNode->id) {
    forwardQueue.push(nextNode);
  } else {
    backwardQueue.push_back(nextNode);
  }
}

void DeltaLockset::handleFunction(GraphNode *startNode) {
  forwardQueue.push(startNode);
  nodeSets = {};
  nodeSets.insert({startNode, {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}}});
  recursive = false;
  bool started = false;
  int lastId = -1;
  std::set<GraphNode *> recursiveVisit = {startNode};

  while (!forwardQueue.empty() || !backwardQueue.empty()) {
    if (forwardQueue.empty()) {
      for (GraphNode *node : backwardQueue) {
        forwardQueue.push(node);
      }
      backwardQueue.clear();
    }

    GraphNode *node = forwardQueue.top();
    EraserSets eraserSet = nodeSets[node];
    forwardQueue.pop();
    if (node->id == lastId) {
      continue;
    }
    lastId = node->id;

    if (node == funcCfgs[currFunc]) {
      if (!started) {
        started = true;
      } else {
        recursive = true;
      }
    }

    std::vector<GraphNode *> nextNodes = node->getNextNodes();
    for (GraphNode *nextNode : nextNodes) {
      EraserSets nextSets = eraserSet;

      if (handleNode(nextNode, nextSets)) {
        if (nodeSets.find(nextNode) == nodeSets.end()) {
          nodeSets.insert({nextNode, nextSets});
          addNodeToQueue(node, nextNode);
        } else {
          nextSets.locks *= nodeSets[nextNode].locks;
          nextSets.unlocks += nodeSets[nextNode].unlocks;

          if (nextSets != nodeSets[nextNode] ||
              (recursive &&
               recursiveVisit.find(nextNode) == recursiveVisit.end())) {
            nodeSets[nextNode] = nextSets;
            addNodeToQueue(node, nextNode);
          }
        }
        if (recursive) {
          recursiveVisit.insert(nextNode);
        }
      }
    }
  }
}

// TODO - startNode unlocks should be used by static eraser to remove from
// current lockset before calling function
void DeltaLockset::updateLocksets(
    std::unordered_map<std::string, StartNode *> funcCfgs,
    std::vector<std::string> changedFunctions) {
  this->funcCfgs = funcCfgs;
  functionSets = {};

  std::vector<std::string> ordering =
      callGraph->deltaLocksetOrdering(changedFunctions);

  // TODO - STORE NEIGHBORS AND PREVIOUS LOCKSETS SO WE KNOW THAT NO CHANGES
  // HAVE NEEDED HAVE CROPPED UP - OPTIMIZATION

  for (const std::string &funcName : ordering) {
    currFunc = funcName;
    handleFunction(funcCfgs[funcName]);

    std::cout << "Function: " << funcName << std::endl;
    std::cout << "Locks: ";
    for (const std::string &lock : functionSets[funcName].locks) {
      std::cout << lock << " ";
    }
    std::cout << std::endl;
    std::cout << "Unlocks: ";
    for (const std::string &lock : functionSets[funcName].unlocks) {
      std::cout << lock << " ";
    }
    std::cout << std::endl;
  }
}