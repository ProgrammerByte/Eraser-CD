#pragma once
#include <iostream>
// #define DEBUG

#ifdef DEBUG
#define debugCout std::cout
#else
#define debugCout                                                              \
  if (false)                                                                   \
  std::cout
#endif
