#include <pthread.h>
#include <stdio.h>

#define COUNT 100
#define THREADS 4

int sum[THREADS] = {0};
int counter = 0;
int largest = 0;
pthread_mutex_t counter_lock;
pthread_mutex_t largest_lock;

void largest_check(int i) {
  pthread_mutex_lock(&largest_lock);
  if (i > largest) {
    largest = i;
  }
  pthread_mutex_unlock(&largest_lock);
}

void *sum_range(void *arg) {
  pthread_mutex_lock(&counter_lock);
  int thread_num = counter++;
  pthread_mutex_unlock(&counter_lock);

  for (int i = thread_num * (COUNT / THREADS);
       i < (thread_num + 1) * (COUNT / THREADS); i++) {

    sum[thread_num] += i;
    largest_check(i);
  }

  return NULL;
}

int main() {
  pthread_t threads[THREADS];

  for (int i = 0; i < THREADS; i++) {
    pthread_create(&threads[i], NULL, sum_range, (void *)NULL);
  }

  for (int i = 0; i < THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  int result = 0;
  for (int i = 0; i < THREADS; i++) {
    result += sum[i];
  }

  printf("Sum is %d", result);

  return 0;
}