#include <pthread.h>

int largest = 0;
pthread_mutex_t largest_lock;

void largest_check(int i) {
  pthread_mutex_lock(&largest_lock);
  if (i > largest) {
    largest = i;
  }
  pthread_mutex_unlock(&largest_lock);
}

int main() { return 0; }