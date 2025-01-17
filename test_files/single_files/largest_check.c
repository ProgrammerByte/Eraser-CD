#include <pthread.h>

int largest = 0;
int smallest = 0;
static int a = 0;
int b = 0;
int c = 0;
int d = 0;

pthread_mutex_t largest_lock;
pthread_mutex_t foo_lock;
pthread_mutex_t recur_lock;

int recur() {
  if (a < 2) {
    a += 1;
    pthread_mutex_unlock(&recur_lock);
    return recur();
  }
  return a;
}

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