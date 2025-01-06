#include "over_approximated_static_eraser.h"
#include "set_operations.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

void OverApproximatedStaticEraser::handleNode(GraphNode *node,
                                              std::set<std::string> &locks) {}

void OverApproximatedStaticEraser::handleNode(ThreadCreateNode *node,
                                              std::set<std::string> &locks) {
  // TODO - node->functionName
}

void OverApproximatedStaticEraser::handleNode(FunctionCallNode *node,
                                              std::set<std::string> &locks) {
  // TODO - node->functionName
}

void OverApproximatedStaticEraser::handleNode(LockNode *node,
                                              std::set<std::string> &locks) {
  locks.insert(node->varName);
}

void OverApproximatedStaticEraser::handleNode(UnlockNode *node,
                                              std::set<std::string> &locks) {
  locks.erase(node->varName);
}

void OverApproximatedStaticEraser::handleNode(ReadNode *node,
                                              std::set<std::string> &locks) {
  std::string varName = node->varName;
  if (this->lockSets.find(varName) == this->lockSets.end()) {
    this->lockSets.insert({varName, locks});
  } else {
    this->lockSets[varName] *= locks;
  }
}

void OverApproximatedStaticEraser::handleNode(WriteNode *node,
                                              std::set<std::string> &locks) {
  // TODO - EXPAND THIS FURTHER
  std::string varName = node->varName;
  if (this->lockSets.find(varName) == this->lockSets.end()) {
    this->lockSets.insert({varName, locks});
  } else {
    this->lockSets[varName] *= locks;
  }
}

void OverApproximatedStaticEraser::handleNode(ReturnNode *node,
                                              std::set<std::string> &locks) {
  // TODO
}

void OverApproximatedStaticEraser::insertNodeIntoQueue(
    GraphNode *node, std::set<std::string> &locks) {
  queue.emplace_back(node, locks);
}

void OverApproximatedStaticEraser::checkNextNodes(
    GraphNode *node, std::set<std::string> &locks) {
  // TODO - USE THESE ARE NEXT NODES TO VISIT
  std::vector<GraphNode *> nextNodes = node->getForwardBranches();
  // if curr->priorNodesVisited == curr->priorNodes and curr->priorNodes > 1
  // then remove from stuck queue in theory though stuck queue not needed
  // for this case - only if paths are not all reachable which may not be
  // the case when using goto

  // TODO - ADD THE NOTION OF INVALIDATING STUFF ALREADY IN THE QUEUE!!!
  for (GraphNode *next : nextNodes) {
    next->priorNodesVisited += 1;
    if (next->priorNodesVisited >= next->priorNodes) {
      if (this->waitingNodeLockSets.find(next) ==
          this->waitingNodeLockSets.end()) {
        queue.emplace_back(next, locks);
      } else {
        queue.emplace_back(next, locks * this->waitingNodeLockSets[next]);
        this->waitingNodeLockSets.erase(next);
      }
      next->priorNodesVisited = 0;

    } else if (this->waitingNodeLockSets.find(next) ==
               this->waitingNodeLockSets.end()) {
      this->waitingNodeLockSets.insert({next, locks});
    } else {
      this->waitingNodeLockSets[next] *= locks;
    }
  }
}

void OverApproximatedStaticEraser::checkNextNodes(
    ThreadCreateNode *node, std::set<std::string> &locks) {
  queue.emplace_back(this->funcCfgs[node->functionName], locks);
}

void OverApproximatedStaticEraser::checkNextNodes(
    FunctionCallNode *node, std::set<std::string> &locks) {
  queue.emplace_back(this->funcCfgs[node->functionName], locks);
}

void OverApproximatedStaticEraser::testFunction(
    std::unordered_map<std::string, StartNode *> funcCfgs,
    std::string funcName) {

  GraphNode *startNode = funcCfgs[funcName];
  queue = {std::make_pair(startNode, std::set<std::string>{})};
  barrierQueue = {}; // TODO - IS THIS NEEDED???
  this->funcCfgs = funcCfgs;
  lockSets = {};
  waitingNodeLockSets = {};

  while (!queue.empty() || !barrierQueue.empty()) {
    if (queue.empty()) {
      // TODO - handle the case where nothing has freed
    } else {
      GraphNode *curr = queue.back().first;
      std::set<std::string> locks = queue.back().second;
      queue.pop_back();

      handleNode(curr, locks);
      checkNextNodes(curr, locks);
    }
  }

  return;
}