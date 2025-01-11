#pragma once

enum NodeType {
  START,
  LOCK,
  UNLOCK,
  READ,
  WRITE,
  FUNCTION_CALL,
  THREAD_CREATE,
  THREAD_JOIN,
  STARTWHILE,
  WHILE,
  ENDWHILE,
  BREAK,
  CONTINUE,
  CONTINUE_RETURN,
  IF,
  ENDIF,
  RETURN
};