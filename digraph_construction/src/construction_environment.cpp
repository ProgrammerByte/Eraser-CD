#include "construction_environment.h"

StartNode *ConstructionEnvironment::startNewTree(std::string funcName) {
  currNode = new StartNode(funcName);
  return (StartNode *)currNode;
};

void ConstructionEnvironment::onAdd(GraphNode *node) {
  if (currNode != nullptr) {
    currNode->add(node);
    currNode = node;
  }
}

void ConstructionEnvironment::callOnAdd(GraphNode *node) { onAdd(node); }

void ConstructionEnvironment::onAdd(IfNode *node) {
  ifStack.push_back(node);
  scopeStack.push_back(node);
  endifListStack.push_back(std::vector<BasicNode *>(0));
  callOnAdd(node);
}

void ConstructionEnvironment::onElseAdd() {
  IfNode *ifNode = ifStack.back();
  ifNode->hasElse = true;
  if (currNode != nullptr && currNode != ifNode) {
    endifListStack.back().push_back((BasicNode *)currNode);
  }
  currNode = ifNode;
}

void ConstructionEnvironment::onAdd(EndifNode *node) {
  IfNode *ifNode = ifStack.back();
  std::vector<BasicNode *> endifNodes = endifListStack.back();
  for (int i = 0; i < endifNodes.size(); i++) {
    endifNodes[i]->add(node);
  }
  if (ifNode->ifNode == nullptr) {
    ifNode->ifNode = node;
  }
  if (!ifNode->hasElse) {
    ifNode->hasElse = true;
    ifNode->add(node);
  }

  ifStack.pop_back();
  scopeStack.pop_back(); // TODO - SCOPE PROBABLY NOT NEEDED
  endifListStack.pop_back();

  if (currNode == nullptr) {
    currNode = node;
  } else {
    callOnAdd(node);
  }
  // TODO - THIS REQUIRES SOME THOUGHT!!!
}

void ConstructionEnvironment::onAdd(StartwhileNode *node) {
  startwhileStack.push_back(node);
  if (!node->isContinueReturn) {
    continueListStack.push_back(std::vector<ContinueNode *>(0));
  }
  if (node->isDoWhile) {
    scopeStack.push_back(node);
    breakListStack.push_back(std::vector<BreakNode *>(0));
  }
  callOnAdd(node);
}

void ConstructionEnvironment::onAdd(WhileNode *node) {
  callOnAdd(node);
  whileStack.push_back(node);
  if (node->isDoWhile) {
    node->whileNode = startwhileStack.back();
  } else {
    scopeStack.push_back(node);
    breakListStack.push_back(std::vector<BreakNode *>(0));
  }
}

void ConstructionEnvironment::onAdd(EndwhileNode *node) {
  std::vector<BreakNode *> breakNodes = breakListStack.back();
  StartwhileNode *startwhileNode = startwhileStack.back();
  WhileNode *whileNode = whileStack.back();
  whileNode->endWhile = node;

  whileStack.pop_back();
  breakListStack.pop_back();
  scopeStack.pop_back();
  startwhileStack.pop_back();

  for (int i = 0; i < breakNodes.size(); i++) {
    breakNodes[i]->next = node;
  }
  node->numBreaks = breakNodes.size();
  currNode = node;
}

void ConstructionEnvironment::onAdd(BreakNode *node) {
  callOnAdd(node);
  breakListStack.back().push_back(node);
  currNode = nullptr;
}

void ConstructionEnvironment::onAdd(ContinueNode *node) {
  callOnAdd(node);
  if (startwhileStack.back()->isContinueReturn) {
    node->next = startwhileStack.back();
  } else {
    continueListStack.back().push_back(node);
  }
  currNode = nullptr;
}

void ConstructionEnvironment::onAdd(ContinueReturnNode *node) {
  std::vector<ContinueNode *> continueNodes = continueListStack.back();
  continueListStack.pop_back();
  for (int i = 0; i < continueNodes.size(); i++) {
    continueNodes[i]->next = node;
  }
  StartwhileNode *startwhileNode = startwhileStack.back();
  currNode = node;
}

void ConstructionEnvironment::onAdd(ReturnNode *node) {
  callOnAdd(node);
  currNode = nullptr;
}