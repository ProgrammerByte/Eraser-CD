#ifndef NODE_TYPES_H
#define NODE_TYPES_H

enum NodeType {
  START,
  LOCK,
  UNLOCK,
  READ,
  WRITE,
  FUNCTION_CALL,
  WHILE,
  ENDWHILE,
  BREAK,
  CONTINUE,
  IF,
  ENDIF,
  RETURN
};

#endif