#include "function_eraser_sets.h"

FunctionEraserSets::FunctionEraserSets(Database *db) : db(db) {
  functionSets = {};
  currFunc = "";
};

void FunctionEraserSets::combineSets(EraserSets &s1, EraserSets &s2) {
  s1.locks *= s2.locks;
  s1.unlocks += s2.unlocks;
  s1.sharedModified += s2.sharedModified;
  // TODO - nit: var can be in both internal and external shared at once if
  // tracking independently, although not the biggest thing
  s1.internalShared += s2.internalShared - s1.sharedModified;
  s1.externalShared += s2.externalShared - s1.sharedModified;
  std::set<std::string> sOrSm =
      s1.sharedModified + s1.internalShared + s1.externalShared; // TODO -
  //    CONSIDER USING THIS!!!
  // std::set<std::string> sOrSm = s1.sharedModified;
  s1.externalReads += s2.externalReads;
  s1.externalReads -= sOrSm;
  s1.externalWrites += s2.externalWrites;
  s1.externalWrites -= sOrSm;
  s1.internalReads += s2.internalReads;
  s1.internalReads -= sOrSm;
  s1.internalWrites += s2.internalWrites;
  s1.internalWrites -= sOrSm;
  s1.queuedWrites += s2.queuedWrites;
  s1.queuedWrites.removeVars(s1.sharedModified);
  s1.activeThreads += s2.activeThreads;
  s1.finishedThreads *= s2.finishedThreads;
}

void FunctionEraserSets::combineSetsForRecursiveThreads(EraserSets &s1,
                                                        EraserSets &s2) {
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
  s1.queuedWrites.removeVars(s1.sharedModified);
  s1.activeThreads += s2.activeThreads;
}

EraserSets *FunctionEraserSets::getEraserSets(std::string funcName) {
  if (funcName == currFunc) {
    return &currFuncSets;
  }
  if (functionSets.find(funcName) != functionSets.end()) {
    return &functionSets[funcName];
  }
  return nullptr;
  // todo - db
}

void FunctionEraserSets::updateCurrEraserSets(EraserSets &sets) {
  if (currFuncSetsStarted) {
    combineSets(currFuncSets, sets);
  } else {
    currFuncSetsStarted = true;
    currFuncSets = sets;
  }
}

void FunctionEraserSets::saveCurrEraserSets() {
  functionSets[currFunc] = currFuncSets;
  // todo - save to db and mark stuff as changed as appropriate
}

void FunctionEraserSets::startNewFunction(std::string funcName) {
  currFunc = funcName;
  currFuncSets = EraserSets::defaultValue;
  currFuncSetsStarted = false;
}