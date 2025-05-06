#include "delta_lockset.h"
#include "debug_tools.h"
#include "set_operations.h"

DeltaLockset::DeltaLockset(CallGraph *callGraph, Parser *parser,
                           FunctionEraserSets *functionEraserSets)
    : callGraph(callGraph), parser(parser),
      functionEraserSets(functionEraserSets) {}

void addVarToSM(std::string varName, EraserSets &sets) {
  sets.sharedModified.insert(varName);
  sets.queuedWrites.removeVar(varName);
  sets.internalReads.erase(varName);
  sets.externalReads.erase(varName);
  sets.internalWrites.erase(varName);
  sets.externalWrites.erase(varName);
  sets.internalShared.erase(varName);
  sets.externalShared.erase(varName);
}

bool DeltaLockset::variableRead(std::string varName, EraserSets &sets) {
  this->functionDirectReads.insert(varName);
  if (sets.sharedModified.find(varName) != sets.sharedModified.end()) {
    return true;
  }
  std::set<std::string> queuedWrites = sets.queuedWrites.values();
  if (queuedWrites.find(varName) != queuedWrites.end()) {
    addVarToSM(varName, sets);
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
}

bool DeltaLockset::variableWrite(std::string varName, EraserSets &sets) {
  this->functionDirectWrites.insert(varName);
  if (sets.sharedModified.find(varName) != sets.sharedModified.end()) {
    return true;
  }
  std::set<std::string> queuedWrites = sets.queuedWrites.values();
  if (queuedWrites.find(varName) != queuedWrites.end() ||
      sets.externalWrites.find(varName) != sets.externalWrites.end() ||
      sets.externalReads.find(varName) != sets.externalReads.end() ||
      sets.externalShared.find(varName) != sets.externalShared.end() ||
      sets.internalShared.find(varName) != sets.internalShared.end()) {
    addVarToSM(varName, sets);
  } else {
    sets.internalWrites.insert(varName);
  }
  sets.activeThreads.erase(varName);
  return true;
}

bool DeltaLockset::recursiveFunctionCall(std::string functionName,
                                         EraserSets &sets,
                                         bool fromThread = false) {
  if (functionName == currFunc) {
    if (sets.eraserIgnoreOn) {
      return !recursive;
    }
    GraphNode *startNode = funcCfgs[currFunc];
    EraserSets nextSets = nodeSets[startNode];
    if (fromThread) {
      functionEraserSets->combineSetsForRecursiveThreads(nextSets, sets);
    } else {
      functionEraserSets->combineSets(nextSets, sets);
      functionEraserSets->saveRecursiveUnlocks(sets.unlocks);
    }
    if (nextSets != nodeSets[startNode] || !recursive) {
      nodeSets[startNode] = nextSets;
      backwardQueue.push_back(startNode);
    }
    return !recursive;
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
    sets.activeThreads.erase(varName);
  }
}

bool DeltaLockset::handleNode(FunctionCallNode *node, EraserSets &sets) {
  std::string functionName = node->functionName;
  if (recursiveFunctionCall(functionName, sets)) {
    return false;
  }
  if (sets.eraserIgnoreOn) {
    return true;
  }

  EraserSets *s1 = &sets;
  EraserSets *s2 = functionEraserSets->getEraserSets(functionName);
  std::set<std::string> s1Shared = s1->externalShared + s1->internalShared;
  std::set<std::string> s2Shared = s2->externalShared + s2->internalShared;
  std::set<std::string> s1Reads = s1->externalReads + s1->internalReads;

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

  s1->queuedWrites += s2->queuedWrites;
  s1->queuedWrites.removeVars(s1->sharedModified);
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
  return true;
};

bool DeltaLockset::handleNode(ThreadCreateNode *node, EraserSets &sets) {
  std::string varName = node->varName;
  if (node->global && varName != "" && !sets.eraserIgnoreOn) {
    variableWrite(varName, sets);
  }
  std::string functionName = node->functionName;
  if (recursiveFunctionCall(node->functionName, sets, true)) {
    return false;
  }
  std::string tid = currFunc + " " + std::to_string(node->id);

  EraserSets *s1 = &sets;
  EraserSets *s2 = functionEraserSets->getEraserSets(functionName);
  std::set<std::string> s1Shared = s1->externalShared + s1->internalShared;
  std::set<std::string> s2Shared = s2->externalShared + s2->internalShared;
  std::set<std::string> s1Reads = s1->externalReads + s1->internalReads;
  std::set<std::string> s2Reads = s2->externalReads + s2->internalReads;
  std::set<std::string> s1Writes = s1->externalWrites + s1->internalWrites +
                                   s1->queuedWrites.values() + s1Shared;
  std::set<std::string> s2Writes = s2->externalWrites + s2->internalWrites +
                                   s2->queuedWrites.values() + s2Shared;

  s1->sharedModified += s2->sharedModified + ((s1Reads + s1Writes) * s2Writes);

  s1->externalShared += s2Shared + s1->sharedModified +
                        ((s1Reads + s1Writes) * (s2Reads + s2Writes));

  s1Shared += s1->externalShared;

  if (s1->queuedWrites.find(tid) == s1->queuedWrites.end()) {
    s1->queuedWrites.insert({tid, {}});
  }
  for (const std::string &write : s2Writes) {
    s1->queuedWrites[tid].insert(write);
  }
  s1->queuedWrites.removeVars(s1->sharedModified);

  s1->internalReads -= s1Shared;
  s1->internalWrites -= s1Shared;
  s1->externalReads += s2->externalReads + s2->internalReads;
  s1->externalReads -= s1Shared;
  s1->externalWrites += s2->externalWrites + s2->internalWrites;
  s1->externalWrites -= s1Shared;
  s1->activeThreads.erase(varName);
  s1->activeThreads -= s2Writes;
  s1->activeThreads += s2->activeThreads;
  if (varName != "") {
    if (s1->activeThreads.find(tid) == s1->activeThreads.end()) {
      s1->activeThreads.insert({varName, {}});
    }
    s1->activeThreads[varName] += {tid};
  }
  s1->externalShared -= s1->sharedModified;

  // If something breaks then try uncommenting the following
  // std::set<std::string> overlap = s1->internalShared * s1->externalShared;
  // s1->internalShared -= overlap;
  // s1->externalShared -= overlap;
  // s1->sharedModified += overlap;
  return true;
};

bool DeltaLockset::handleNode(ThreadJoinNode *node, EraserSets &sets) {
  std::string varName = node->varName;
  if (node->global && varName != "" && !sets.eraserIgnoreOn) {
    variableRead(varName, sets);
  }
  threadFinished(varName, sets);
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
  if (sets.eraserIgnoreOn) {
    return true;
  }
  return variableRead(node->varName, sets);
};

bool DeltaLockset::handleNode(WriteNode *node, EraserSets &sets) {
  if (sets.eraserIgnoreOn) {
    return true;
  }
  return variableWrite(node->varName, sets);
};

bool DeltaLockset::handleNode(ReturnNode *node, EraserSets &sets) {
  functionEraserSets->updateCurrEraserSets(sets);
  return true;
}

bool DeltaLockset::handleNode(EraserIgnoreOnNode *node, EraserSets &sets) {
  sets.eraserIgnoreOn = true;
  return true;
}

bool DeltaLockset::handleNode(EraserIgnoreOffNode *node, EraserSets &sets) {
  sets.eraserIgnoreOn = false;
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
  } else if (auto *eraserIgnoreOnNode =
                 dynamic_cast<EraserIgnoreOnNode *>(node)) {
    return handleNode(eraserIgnoreOnNode, sets);
  } else if (auto *eraserIgnoreOffNode =
                 dynamic_cast<EraserIgnoreOffNode *>(node)) {
    return handleNode(eraserIgnoreOffNode, sets);
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
  functionEraserSets->startNewFunction(currFunc);
  forwardQueue.push(startNode);
  nodeSets.insert({startNode, EraserSets::defaultValue});
  nodeSets[startNode].eraserIgnoreOn = false;
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
          functionEraserSets->combineSets(nextSets, nodeSets[nextNode]);

          if (nextSets != nodeSets[nextNode] ||
              (recursive &&
               recursiveVisit.find(nextNode) == recursiveVisit.end())) {
            nodeSets[nextNode] = nextSets;
            addNodeToQueue(node, nextNode);
          }
        }
        nextNode->eraserIgnoreOn = nextSets.eraserIgnoreOn;
        if (recursive) {
          recursiveVisit.insert(nextNode);
        }
      }
    }
  }
  functionEraserSets->saveFunctionDirectVariableAccesses(functionDirectReads,
                                                         functionDirectWrites);
  functionEraserSets->saveCurrEraserSets();

  functionDirectReads.clear();
  functionDirectWrites.clear();
  nodeSets.clear();
}

void DeltaLockset::updateLocksets(std::vector<std::string> changedFunctions) {
  std::vector<std::string> ordering =
      callGraph->deltaLocksetOrdering(changedFunctions);

  for (const std::string &funcName : ordering) {
    if (!callGraph->shouldVisitNode(funcName)) {
      debugCout << "DL SKIPPING " << funcName << std::endl;
      continue;
    }
    debugCout << "DL Looking At " << funcName << std::endl;
    currFunc = funcName;
    if (funcCfgs.find(funcName) == funcCfgs.end()) {
      std::string fileName = callGraph->getFilenameFromFuncname(funcName);
      parser->parseFile(fileName.c_str());
    }
    handleFunction(funcCfgs[funcName]);

    if (false) {
      EraserSets *sets = functionEraserSets->getEraserSets(funcName);
      std::cout << "Function: " << funcName << std::endl;
      std::cout << "Locks: ";
      for (const std::string &lock : sets->locks) {
        std::cout << lock << ", ";
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
    }
  }
}