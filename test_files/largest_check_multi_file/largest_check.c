#include "largest_check.h"
#include "globals.h"

int largest = 0;

void largest_check(int i) {
  pthread_mutex_lock(&recur_lock);
  recur();
  // pthread_mutex_lock(&largest_lock);
  if (i > largest) {
    // largest += 1;
    return;
  } else if (i < largest) {
    // pthread_mutex_unlock(&largest_lock);
    return;
  }
  pthread_mutex_unlock(&foo_lock);
}