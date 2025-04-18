

/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

#include <assert.h>
#include <semaphore.h>
#include <stdlib.h>
#if __STDC_VERSION__ >= 201112L
#include <stdatomic.h>
#endif
#include <stdint.h>

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <sys/time.h>

#define PAGE_SIZE 4096
#define __MAX_THREADS__ 256

extern pthread_t __tid__[__MAX_THREADS__];
extern unsigned __threads__;
extern pthread_mutex_t __intern__;

extern unsigned __count__;
extern volatile int __sense__;
extern __thread int __local_sense__;

#include "matrix.h"
#include <math.h>

double max_block_size;
extern long *node;
long *boundary, *next_in_segment, *next_segment, *sets_affected, n_affected;
long *partition;
long *segment_perm;

void ComputeTargetBlockSize(SMatrix M, long P) {
  long max_ht;
  double total_ops;
  extern double *work_tree;

  max_ht = 0;
  FindMaxHeight(M, M.n, 0, &max_ht);

  total_ops = work_tree[M.n];

  max_block_size = sqrt(total_ops / (3 * max_ht) / P);

  printf("%ld max height, %.0f ops, %.2f conc, %.2f bl for %ld P\n", max_ht,
         total_ops, total_ops / (3 * max_ht), max_block_size, P);
}

void FindMaxHeight(SMatrix L, long root, long height, long *maxm) {
  long i;
  extern long *firstchild, *child;

  if (height > *maxm)
    *maxm = height;

  for (i = firstchild[root]; i < firstchild[root + 1]; i++)
    FindMaxHeight(L, child[i], height + 1, maxm);
}

void NoSegments(SMatrix M) {
  long i;

  partition = (long *)MyMalloc(M.n * sizeof(long), DISTRIBUTED);
  for (i = 0; i < M.n; i++)
    partition[i] = node[i];
}

void CreatePermutation(long n, long *PERM, long permutation_method) {
  long j;

  PERM[n] = n;
  if (permutation_method == NO_PERM) {
    for (j = 0; j < n; j++)
      PERM[j] = j;
  }
}
