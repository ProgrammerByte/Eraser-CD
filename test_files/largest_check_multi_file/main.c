#include "globals.h"
#include "largest_check.h"
#include "recur.h"
#include <pthread.h>

int smallest = 0;
int b = 0;
int c = 0;
int d = 0;

int main() {
  pthread_t threads[100];

  for (int i = 0; i < 100; i++) {
    pthread_create(&threads[i], NULL, largest_check, (void *)NULL);
  }

  for (int i = 0; i < 100; i++) {
    pthread_join(threads[i], NULL);
  }

  return 0;
}