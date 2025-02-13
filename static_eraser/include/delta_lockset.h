#pragma once
#include "break_node.h"
#include "call_graph.h"
#include "continue_node.h"
#include "endif_node.h"
#include "endwhile_node.h"
#include "eraser_sets.h"
#include "function_call_node.h"
#include "function_eraser_sets.h"
#include "graph_node.h"
#include "if_node.h"
#include "lock_node.h"
#include "parser.h"
#include "read_node.h"
#include "return_node.h"
#include "set_operations.h"
#include "start_node.h"
#include "thread_create_node.h"
#include "thread_join_node.h"
#include "unlock_node.h"
#include "while_node.h"
#include "write_node.h"

class DeltaLockset {
public:
  explicit DeltaLockset(CallGraph *callGraph, Parser *parser,
                        FunctionEraserSets *functionEraserSets);
  virtual ~DeltaLockset() = default;

  void updateLocksets(std::vector<std::string> changedFunctions);

private:
  bool recursive;
  CallGraph *callGraph;
  Parser *parser;
  FunctionEraserSets *functionEraserSets;
  std::set<std::string> functionDirectReads = {};
  std::set<std::string> functionDirectWrites = {};

  std::priority_queue<GraphNode *, std::vector<GraphNode *>, CompareGraphNode>
      forwardQueue = {};

  std::vector<GraphNode *> backwardQueue = {};

  std::string currFunc;
  std::unordered_map<GraphNode *, EraserSets> nodeSets = {};

  bool variableRead(std::string varName, EraserSets &sets);
  bool variableWrite(std::string varName, EraserSets &sets);
  bool recursiveFunctionCall(std::string functionName, EraserSets &sets,
                             bool fromThread);

  void threadFinished(std::string varName, EraserSets &sets);

  bool handleNode(FunctionCallNode *node, EraserSets &sets);
  bool handleNode(ThreadCreateNode *node, EraserSets &sets);
  bool handleNode(ThreadJoinNode *node, EraserSets &sets);
  bool handleNode(LockNode *node, EraserSets &sets);
  bool handleNode(UnlockNode *node, EraserSets &sets);
  bool handleNode(ReadNode *node, EraserSets &sets);
  bool handleNode(WriteNode *node, EraserSets &sets);
  bool handleNode(ReturnNode *node, EraserSets &sets);
  bool handleNode(EraserIgnoreOnNode *node, EraserSets &sets);
  bool handleNode(EraserIgnoreOffNode *node, EraserSets &sets);
  bool handleNode(GraphNode *node, EraserSets &sets);

  void addNodeToQueue(GraphNode *startNode, GraphNode *nextNode);
  void handleFunction(GraphNode *startNode);
};