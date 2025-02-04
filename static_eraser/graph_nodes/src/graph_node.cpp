#include "graph_node.h"
#include "node_types.h"

GraphNode::GraphNode(NodeType type) : type(type) {}

std::string GraphNode::getPrintableNameWithId() {
  return std::to_string(id) + " " + getPrintableName();
}

std::string GraphNode::getNodeType() {
  switch (type) {
  case START:
    return "START";
  case LOCK:
    return "LOCK";
  case UNLOCK:
    return "UNLOCK";
  case READ:
    return "READ";
  case WRITE:
    return "WRITE";
  case FUNCTION_CALL:
    return "FUNCTION_CALL";
  case WHILE:
    return "WHILE";
  case ENDWHILE:
    return "ENDWHILE";
  case BREAK:
    return "BREAK";
  case CONTINUE:
    return "CONTINUE";
  case IF:
    return "IF";
  case ENDIF:
    return "ENDIF";
  case RETURN:
    return "RETURN";
  default:
    break;
  }
  return "UNKNOWN";
}