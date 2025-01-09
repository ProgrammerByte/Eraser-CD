#include "delta_lockset.h"
#include "set_operations.h"

DeltaLockset::DeltaLockset(CallGraph *callGraph) {
  this->callGraph = callGraph;
}

void DeltaLockset::handleNode(GraphNode *node, std::set<std::string> &locks){};

void DeltaLockset::handleNode(FunctionCallNode *node,
                              std::set<std::string> &locks) {
  std::string functionName = node->functionName;
  if (deltaLocksets.find(functionName) != deltaLocksets.end()) {
    locks *= deltaLocksets[functionName];
  }
};

void DeltaLockset::handleNode(LockNode *node, std::set<std::string> &locks) {
  locks.insert(node->varName);
};

void DeltaLockset::handleNode(UnlockNode *node, std::set<std::string> &locks) {
  locks.erase(node->varName);
};

void DeltaLockset::handleNode(ReturnNode *node, std::set<std::string> &locks) {
  if (deltaLocksets.find(currFunc) == deltaLocksets.end()) {
    deltaLocksets.insert({currFunc, locks});
  } else {
    deltaLocksets[currFunc] *= locks;
  }
}

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
    std::set<std::string> lockSet = nodeLockSets[node];
    forwardQueue.pop();

    std::vector<GraphNode *> nextNodes = node->getNextNodes();
    for (GraphNode *nextNode : nextNodes) {
      std::set<std::string> nextLockSet = lockSet;
      handleNode(nextNode, nextLockSet);

      if (nodeLockSets.find(nextNode) == nodeLockSets.end()) {
        nodeLockSets.insert({nextNode, nextLockSet});
        addNodeToQueue(node, nextNode);
      } else {
        nextLockSet *= nodeLockSets[nextNode];

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
  }
}