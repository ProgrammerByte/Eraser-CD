#include "recur.h"
#include "globals.h"

static int a = 0;

int recur() {
  if (a < 2) {
    a += 1;
    pthread_mutex_unlock(&recur_lock);
    return recur();
  }
  return a;
}