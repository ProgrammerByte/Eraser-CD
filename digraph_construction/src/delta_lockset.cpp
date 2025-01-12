#include "delta_lockset.h"
#include "set_operations.h"

DeltaLockset::DeltaLockset(CallGraph *callGraph) {
  this->callGraph = callGraph;
}

void combineSets(EraserSets &s1, EraserSets &s2) {
  s1.locks *= s2.locks;
  s1.unlocks += s2.unlocks;
  s1.sharedModified += s2.sharedModified;
  // TODO - nit: var can be in both internal and external shared at once if
  // tracking independently, although not the biggest thing
  s1.internalShared += s2.internalShared;
  s1.externalShared += s2.externalShared;
  // std::set<std::string> sOrSm =
  //    s1.sharedModified + s1.internalShared + s1.externalShared; // TODO -
  //    CONSIDER USING THIS!!!
  std::set<std::string> sOrSm = s1.sharedModified;
  s1.externalReads += s2.externalReads;
  s1.externalReads -= sOrSm;
  s1.externalWrites += s2.externalWrites;
  s1.externalWrites -= sOrSm;
  s1.internalReads += s2.internalReads;
  s1.internalReads -= sOrSm;
  s1.internalWrites += s2.internalWrites;
  s1.internalWrites -= sOrSm;
  s1.queuedWrites += s2.queuedWrites;
  // TODO - AGAIN CONSIDER REMOVING SM FROM QUEUE
  s1.activeThreads += s2.activeThreads;
  s1.finishedThreads *= s2.finishedThreads;
}

void combineSetsForRecursiveThreads(EraserSets &s1, EraserSets &s2) {
  s1.sharedModified += s2.sharedModified;
  s1.externalShared += s2.internalShared + s2.externalShared;
  // std::set<std::string> sOrSm =
  //    s1.sharedModified + s1.internalShared + s1.externalShared; // TODO -
  //    CONSIDER USING THIS!!!
  std::set<std::string> sOrSm = s1.sharedModified;
  s1.externalReads += s2.internalReads + s2.externalReads;
  s1.externalReads -= sOrSm;
  s1.externalWrites += s2.internalWrites + s2.externalWrites;
  s1.externalWrites -= sOrSm;
  s1.queuedWrites += s2.queuedWrites;
  // TODO - AGAIN CONSIDER REMOVING SM FROM QUEUE
  s1.activeThreads += s2.activeThreads;
}

bool DeltaLockset::recursiveFunctionCall(std::string functionName,
                                         EraserSets &sets,
                                         bool fromThread = false) {
  if (functionName == currFunc) {
    GraphNode *startNode = funcCfgs[currFunc];
    EraserSets nextSets = nodeSets[startNode];
    if (fromThread) {
      combineSetsForRecursiveThreads(nextSets, sets);
    } else {
      combineSets(nextSets, sets);
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

void DeltaLockset::threadFinished(std::string varName, EraserSets &sets) {
  if (sets.activeThreads.find(varName) == sets.activeThreads.end()) {
    sets.finishedThreads.insert(varName);
  } else {
    std::set<std::string> tids = sets.activeThreads[varName];
    for (const std::string &tid : tids) {
      sets.externalWrites += sets.queuedWrites[tid];
    }
    sets.queuedWrites -= tids;
  }
}

bool DeltaLockset::handleNode(FunctionCallNode *node, EraserSets &sets) {
  std::string functionName = node->functionName;
  if (recursiveFunctionCall(functionName, sets)) {
    return false;
  }
  if (functionSets.find(functionName) != functionSets.end()) {
    EraserSets *s1 = &sets;
    EraserSets *s2 = &functionSets[functionName];
    std::set<std::string> s1Shared = s1->externalShared + s1->internalShared;
    std::set<std::string> s2Shared = s2->externalShared + s2->internalShared;
    std::set<std::string> s1Reads = s1->externalReads + s1->internalReads;
    std::set<std::string> s2Reads = // TODO - NOT USED???
        s2->externalReads + s2->internalReads;

    std::set<std::string> s1ExternalWrites =
        s1->externalWrites + s1->queuedWrites.values() + s1->externalShared;
    std::set<std::string> s1InternalWrites =
        s1->internalWrites + s1->internalShared;
    std::set<std::string> s1Writes = s1InternalWrites + s1ExternalWrites;
    std::set<std::string> s2ExternalWrites =
        s2->externalWrites + s2->queuedWrites.values() + s2->externalShared;
    std::set<std::string> s2InternalWrites =
        s2->internalWrites + s2->internalShared;
    std::set<std::string> s2Writes = s2InternalWrites + s2ExternalWrites;

    s1->locks -= s2->unlocks;
    s1->locks += s2->locks;
    s1->unlocks -= s2->locks;
    s1->unlocks += s2->unlocks;

    s1->sharedModified +=
        s2->sharedModified + ((s1Reads + s1Writes) * s2ExternalWrites) +
        ((s1ExternalWrites + s1->externalReads + s1Shared) * s2Writes);

    s1->internalShared +=
        s2->internalShared + (s1InternalWrites + s2InternalWrites) *
                                 (s1ExternalWrites + s2ExternalWrites +
                                  s1->externalReads + s2->externalReads);

    s1->externalShared +=
        s2->externalShared + (s1ExternalWrites + s2ExternalWrites) *
                                 (s1->internalWrites + s2->internalWrites +
                                  s1->internalReads + s2->internalReads);

    std::set<std::string> overlap = s1->internalShared * s1->externalShared;
    s1->internalShared -= overlap;
    s1->externalShared -= overlap;
    s1->sharedModified += overlap;

    std::set<std::string> sOrSm =
        s1->internalShared + s1->externalShared + s1->sharedModified;

    s1->queuedWrites +=
        s2->queuedWrites; // TODO - AGAIN NOTION OF REMOVING SM FROM QUEUE
    s1->internalReads += s2->internalReads;
    s1->internalReads -= sOrSm;
    s1->internalWrites += s2->internalWrites;
    s1->internalWrites -= sOrSm;
    s1->externalReads += s2->externalReads;
    s1->externalReads -= sOrSm;
    s1->externalWrites += s2->externalWrites;

    for (const std::string &varName : s2->finishedThreads) {
      threadFinished(varName, *s1);
    }

    s1->internalShared -= s1->sharedModified;
    s1->externalShared -= s1->sharedModified;
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
  if (functionSets.find(functionName) != functionSets.end()) {
    std::string tid = currFunc + std::to_string(node->id);

    EraserSets *s1 = &sets;
    EraserSets *s2 = &functionSets[functionName];
    std::set<std::string> s1Shared = s1->externalShared + s1->internalShared;
    std::set<std::string> s2Shared = s2->externalShared + s2->internalShared;
    std::set<std::string> s1Reads = s1->externalReads + s1->internalReads;
    std::set<std::string> s2Reads = s2->externalReads + s2->internalReads;
    std::set<std::string> s1Writes = s1->externalWrites + s1->internalWrites +
                                     s1->queuedWrites.values() + s1Shared;
    std::set<std::string> s2Writes = s2->externalWrites + s2->internalWrites +
                                     s2->queuedWrites.values() + s2Shared;

    s1->sharedModified +=
        s2->sharedModified + ((s1Reads + s1Writes) * s2Writes);

    s1->externalShared += s2Shared + s1->sharedModified +
                          ((s1Reads + s1Writes) * (s2Reads + s2Writes));

    s1Shared += s1->externalShared;

    if (s1->queuedWrites.find(tid) == s1->queuedWrites.end()) {
      s1->queuedWrites.insert({tid, {}});
    }
    for (const std::string &write : s2Writes) {
      s1->queuedWrites[tid].insert(write);
    }
    // TODO - not super important but remove varnames from queue that are SM
    // here!!!

    s1->internalReads -= s1Shared;
    s1->internalWrites -= s1Shared;
    s1->externalReads += s2->externalReads + s2->internalReads;
    s1->externalReads -= s1Shared;
    s1->externalWrites += s2->externalWrites + s2->internalWrites;
    s1->externalWrites -= s1Shared;
    s1->activeThreads -= s2Writes;
    s1->activeThreads += s2->activeThreads;
    if (s1->activeThreads.find(tid) == s1->activeThreads.end()) {
      s1->activeThreads.insert({tid, {}});
    }
    s1->activeThreads[tid] += {functionName};
    s1->externalShared -= s1->sharedModified;

    // If something breaks then try uncommenting the following
    // std::set<std::string> overlap = s1->internalShared * s1->externalShared;
    // s1->internalShared -= overlap;
    // s1->externalShared -= overlap;
    // s1->sharedModified += overlap;
  }
  return true;
};

bool DeltaLockset::handleNode(ThreadJoinNode *node, EraserSets &sets) {
  threadFinished(node->varName, sets);
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
  std::string varName = node->varName;
  if (sets.sharedModified.find(varName) != sets.sharedModified.end()) {
    return true;
  }
  std::set<std::string> queuedWrites = sets.queuedWrites.values();
  if (queuedWrites.find(varName) != queuedWrites.end()) {
    sets.sharedModified.insert(varName);
    // TODO - nit: remove from Q
  } else if (sets.externalReads.find(varName) != sets.externalReads.end() ||
             sets.externalWrites.find(varName) != sets.externalWrites.end()) {
    sets.externalShared.insert(varName);
    sets.externalWrites.erase(varName);
    sets.externalReads.erase(varName);
  } else if (sets.externalShared.find(varName) == sets.externalShared.end() ||
             sets.internalShared.find(varName) == sets.internalShared.end()) {
    sets.internalReads.insert(varName);
  }
  return true;
};

bool DeltaLockset::handleNode(WriteNode *node, EraserSets &sets) {
  std::string varName = node->varName;
  if (sets.sharedModified.find(varName) != sets.sharedModified.end()) {
    return true;
  }
  std::set<std::string> queuedWrites = sets.queuedWrites.values();
  if (queuedWrites.find(varName) != queuedWrites.end() ||
      sets.externalWrites.find(varName) != sets.externalWrites.end() ||
      sets.externalReads.find(varName) != sets.externalReads.end() ||
      sets.externalShared.find(varName) != sets.externalShared.end() ||
      sets.internalShared.find(varName) != sets.internalShared.end()) {
    sets.sharedModified.insert(varName);
    // TODO - nit: remove from Q
    sets.externalWrites.erase(varName);
    sets.externalReads.erase(varName);
    sets.externalShared.erase(varName);
    sets.internalShared.erase(varName);
  } else {
    sets.internalWrites.insert(varName);
  }
  sets.activeThreads.erase(varName);
  return true;
};

bool DeltaLockset::handleNode(ReturnNode *node, EraserSets &sets) {
  if (functionSets.find(currFunc) == functionSets.end()) {
    functionSets.insert({currFunc, sets});
  } else {
    combineSets(functionSets[currFunc], sets);
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
  nodeSets.insert(
      {startNode, {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}}});
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
          combineSets(nextSets, nodeSets[nextNode]);

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