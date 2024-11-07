#pragma once

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