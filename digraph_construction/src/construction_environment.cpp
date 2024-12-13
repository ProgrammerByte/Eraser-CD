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
  if (currNode != nullptr) {
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

void ConstructionEnvironment::onAdd(WhileNode *node) {
  whileStack.push_back(node);
  scopeStack.push_back(node);
  breakListStack.push_back(std::vector<BreakNode *>(0));
  callOnAdd(node);
}

void ConstructionEnvironment::onAdd(EndwhileNode *node) {
  std::vector<BreakNode *> breakNodes = breakListStack.back();
  WhileNode *whileNode = whileStack.back();
  breakListStack.pop_back();
  scopeStack.pop_back();
  whileStack.pop_back();

  for (int i = 0; i < breakNodes.size(); i++) {
    breakNodes[i]->next = node;
  }
  node->numBreaks = breakNodes.size();
  whileNode->endWhile = node;
  if (currNode == nullptr) {
    currNode = node;
  } else {
    callOnAdd(node);
  }
}

// TODO - IMPLEMENT THESE TWO!!!
void ConstructionEnvironment::onAdd(BreakNode *node) {
  callOnAdd(node);
  breakListStack.back().push_back(node);
  currNode = nullptr;
}

void ConstructionEnvironment::onAdd(ContinueNode *node) {
  callOnAdd(node);
  node->next = whileStack.back();
  currNode = nullptr;
}

void ConstructionEnvironment::onAdd(ReturnNode *node) {
  callOnAdd(node);
  currNode = nullptr;
}