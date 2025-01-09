#include "delta_lockset.h"
#include "set_operations.h"

DeltaLockset::DeltaLockset(CallGraph *callGraph) {
  this->callGraph = callGraph;
}

void DeltaLockset::handleNode(
    FunctionCallNode *node,
    std::pair<std::set<std::string>, std::set<std::string>> &locks) {
  std::string functionName = node->functionName;
  if (deltaLocksets.find(functionName) != deltaLocksets.end()) {
    locks.first += deltaLocksets[functionName].first;
    locks.second += deltaLocksets[functionName].second;
  }
};

void DeltaLockset::handleNode(
    LockNode *node,
    std::pair<std::set<std::string>, std::set<std::string>> &locks) {
  locks.first.insert(node->varName);
  locks.second.erase(node->varName);
};

void DeltaLockset::handleNode(
    UnlockNode *node,
    std::pair<std::set<std::string>, std::set<std::string>> &locks) {
  locks.first.erase(node->varName);
  locks.second.insert(node->varName);
};

void DeltaLockset::handleNode(
    ReturnNode *node,
    std::pair<std::set<std::string>, std::set<std::string>> &locks) {
  if (deltaLocksets.find(currFunc) == deltaLocksets.end()) {
    deltaLocksets.insert({currFunc, locks});
  } else {
    deltaLocksets[currFunc].first *= locks.first;
    deltaLocksets[currFunc].second += locks.second;
  }
}

void DeltaLockset::handleNode(
    GraphNode *node,
    std::pair<std::set<std::string>, std::set<std::string>> &locks) {
  if (auto *functionNode = dynamic_cast<FunctionCallNode *>(node)) {
    handleNode(functionNode, locks);
  } else if (auto *lockNode = dynamic_cast<LockNode *>(node)) {
    handleNode(lockNode, locks);
  } else if (auto *unlockNode = dynamic_cast<UnlockNode *>(node)) {
    handleNode(unlockNode, locks);
  } else if (auto *returnNode = dynamic_cast<ReturnNode *>(node)) {
    handleNode(returnNode, locks);
  }
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
  nodeLockSets = {};
  nodeLockSets.insert({startNode, {}});

  while (!forwardQueue.empty() || !backwardQueue.empty()) {
    if (forwardQueue.empty()) {
      for (GraphNode *node : backwardQueue) {
        forwardQueue.push(node);
      }
      backwardQueue.clear();
    }

    GraphNode *node = forwardQueue.top();
    std::pair<std::set<std::string>, std::set<std::string>> lockSet =
        nodeLockSets[node];
    forwardQueue.pop();

    std::vector<GraphNode *> nextNodes = node->getNextNodes();
    for (GraphNode *nextNode : nextNodes) {
      std::pair<std::set<std::string>, std::set<std::string>> nextLockSet =
          lockSet;

      handleNode(nextNode, nextLockSet);

      if (nodeLockSets.find(nextNode) == nodeLockSets.end()) {
        nodeLockSets.insert({nextNode, nextLockSet});
        addNodeToQueue(node, nextNode);
      } else {
        nextLockSet.first *= nodeLockSets[nextNode].first;
        nextLockSet.second += nodeLockSets[nextNode].second;

        if (nextLockSet != nodeLockSets[nextNode]) {
          nodeLockSets[nextNode] = nextLockSet;
          addNodeToQueue(node, nextNode);
        }
      }
    }
  }
}

void DeltaLockset::updateLocksets(
    std::unordered_map<std::string, StartNode *> funcCfgs,
    std::vector<std::string> changedFunctions) {
  this->funcCfgs = funcCfgs;
  deltaLocksets = {};

  std::vector<std::string> ordering =
      callGraph->deltaLocksetOrdering(changedFunctions);

  // TODO - STORE NEIGHBORS AND PREVIOUS LOCKSETS SO WE KNOW THAT NO CHANGES
  // HAVE NEEDED HAVE CROPPED UP - OPTIMIZATION

  for (const std::string &funcName : ordering) {
    currFunc = funcName;
    handleFunction(funcCfgs[funcName]);

    std::cout << "Function: " << funcName << std::endl;
    std::cout << "Locks: ";
    for (const std::string &lock : deltaLocksets[funcName].first) {
      std::cout << lock << " ";
    }
    std::cout << std::endl;
    std::cout << "Unlocks: ";
    for (const std::string &lock : deltaLocksets[funcName].second) {
      std::cout << lock << " ";
    }
    std::cout << std::endl;
  }
}