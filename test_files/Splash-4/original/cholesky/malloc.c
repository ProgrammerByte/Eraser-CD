

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

/* this version guarentees that all blocks are zeroed, and it expects
   that all returned blocks are zero */

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

#define ALIGN 8
#define MAXFAST 16
#define MAXFASTBL (1 << (MAXFAST - 1))

#define SIZE(block) (((long *)block)[-1])   /* in all blocks */
#define HOME(block) (((long *)block)[-2])   /* in all blocks */
#define NEXTFREE(block) (*((long **)block)) /* in free blocks */

struct MemPool {
  pthread_mutex_t memoryLock;
  long **freeBlock;
  long tally, touched, maxm;
} * mem_pool;

long mallocP = 1, machineP = 1;

extern struct GlobalMemory *Global;

void MallocInit(long P) {
  long p;

  mallocP = P;

  mem_pool = (struct MemPool *)({
    void *mem = malloc((mallocP + 1) * sizeof(struct MemPool));
    assert(mem);
    mem;
  });
  ;
  mem_pool++;
  /****** access to mem_pool[-1] is valid ******/

  for (p = -1; p < mallocP; p++)
    InitOneFreeList(p);
}

void InitOneFreeList(long p) {
  long j;

  { pthread_mutex_init(&(mem_pool[p].memoryLock), NULL); };
  if (p > 0) {
    mem_pool[p].freeBlock = (long **)({
      void *mem = malloc((MAXFAST + 1) * sizeof(long *));
      assert(mem);
      mem;
    });
    ;
    MigrateMem(mem_pool[p].freeBlock, (MAXFAST + 1) * sizeof(long *), p);
  } else {
    mem_pool[p].freeBlock = (long **)({
      void *mem = malloc((MAXFAST + 1) * sizeof(long *));
      assert(mem);
      mem;
    });
    ;
    MigrateMem(mem_pool[p].freeBlock, (MAXFAST + 1) * sizeof(long *),
               DISTRIBUTED);
  }
  for (j = 0; j <= MAXFAST; j++)
    mem_pool[p].freeBlock[j] = NULL;
  mem_pool[p].tally = mem_pool[p].maxm = mem_pool[p].touched = 0;
}

void MallocStats() {
  long i;

  printf("Malloc max: ");
  for (i = 0; i < mallocP; i++)
    if (mem_pool[i].touched > 0)
      printf("%ld* ", mem_pool[i].maxm);
    else
      printf("%ld ", mem_pool[i].maxm);
  printf("\n");
}

/* returns first bucket where 2^bucket >= size */

long FindBucket(long size) {
  long bucket;

  if (size > MAXFASTBL)
    bucket = MAXFAST;
  else {
    bucket = 1;
    while (1 << bucket < size)
      bucket++;
  }

  return (bucket);
}

/* size is in bytes */

char *MyMalloc(long size, long home) {
  long i, bucket, leftover, alloc_size, block_size;
  long *d, *result, *prev, *freespace;

  if (size < ALIGN)
    size = ALIGN;

  if (home == DISTRIBUTED)
    home = -1;

  bucket = FindBucket(size);

  if (bucket < MAXFAST)
    alloc_size = 1 << bucket;
  else
    alloc_size = ((size + ALIGN - 1) / ALIGN) * ALIGN;

  result = NULL;

  if (bucket < MAXFAST) { /*
     {; pthread_mutex_lock(&(mem_pool[home].memoryLock)); ;}
     result = mem_pool[home].freeBlock[bucket];
     if (result)
       mem_pool[home].freeBlock[bucket] = NEXTFREE(result);
     {; pthread_mutex_unlock(&(mem_pool[home].memoryLock)); ;}*/

    result = atomic_load(&(mem_pool[home].freeBlock[bucket]));
    do {
      if (!result)
        break;
    } while (!({
      ;
      _Bool ___b = atomic_compare_exchange_weak(
          &(mem_pool[home].freeBlock[bucket]), &(result), NEXTFREE(result));
      ;
      ___b;
    }));
  }

  if (!result) {
    {
      ;
      pthread_mutex_lock(&(mem_pool[home].memoryLock));
      ;
    }
    prev = NULL;
    d = mem_pool[home].freeBlock[MAXFAST];
    while (d) {

      block_size = SIZE(d);

      if (block_size >= alloc_size + 2 * sizeof(long)) { /* Found one! */

        leftover = block_size - alloc_size - 2 * sizeof(long);
        result = d + (leftover / sizeof(long)) + 2;
        SIZE(result) = alloc_size;
        HOME(result) = home;

        if (leftover > MAXFASTBL) {
          SIZE(d) = leftover;
        } else {
          /* don't leave a block */

          /* unlink 'd' */
          if (prev)
            NEXTFREE(prev) = NEXTFREE(d);
          else
            mem_pool[home].freeBlock[MAXFAST] = NEXTFREE(d);

          if (leftover > 0) {
            SIZE(d) = leftover;
            MyFreeNow(d);
          }
        }
        break;
      }

      prev = d;
      d = NEXTFREE(d);
    }

    {
      ;
      pthread_mutex_unlock(&(mem_pool[home].memoryLock));
      ;
    }
  }

  if (result) {
    NEXTFREE(result) = 0; /* in case it was set earlier */
  } else {
    /* grab a big block, free it, then retry request */
    block_size = max(alloc_size, 4 * (1 << MAXFAST));
    {
      ;
      pthread_mutex_lock(&(Global->memLock));
      ;
    };
    // XXX WARNING: Violation of the strict aliasing rule
    freespace = (long *)({
      void *mem = malloc(block_size + 2 * sizeof(long));
      assert(mem);
      mem;
    });
    ;
    MigrateMem(freespace, block_size + 2 * sizeof(long), home);

    mem_pool[home].touched++;
    {
      ;
      pthread_mutex_unlock(&(Global->memLock));
      ;
    };
    freespace += 2;
    SIZE(freespace) = block_size;
    HOME(freespace) = home;
    for (i = 0; i < block_size / sizeof(long); i++)
      freespace[i] = 0;
    if (block_size == alloc_size)
      result = freespace;
    else {
      mem_pool[home].tally += SIZE(freespace);
      if (mem_pool[home].tally > mem_pool[home].maxm) {
        mem_pool[home].maxm = mem_pool[home].tally;
      }
      MyFree(freespace);
      result = (long *)MyMalloc(alloc_size, home);
    }
  }

  mem_pool[home].tally += SIZE(result);
  if (mem_pool[home].tally > mem_pool[home].maxm) {
    mem_pool[home].maxm = mem_pool[home].tally;
  }

  if (SIZE(result) < size)
    printf("*** Bad size from malloc %ld, %ld\n", size, SIZE(result));

  return ((char *)result);
}

void MigrateMem(void *start, long length, long home) {
  /*  unsigned long *finish;
    unsigned long currpage, endpage;
    long i;
    long j;*/

  /* POSSIBLE ENHANCEMENT:  Here is where one might distribute the memory
     pages across physically distributed memories as desired.

     One way to do this is as follows:

     finish = (unsigned long *) (((char *) start) + length);

     currpage = (unsigned long) start;
     endpage = (unsigned long) finish;
     if ((home == DISTRIBUTED) || (home < 0) || (home >= mallocP)) {
       j = 0;
       while (currpage < endpage) {

         Place all addresses x such that
           (currpage <= x < currpage + PAGE_SIZE) on node j

         j++;
         currpage += PAGE_SIZE;
       }
     } else {
       Place all addresses x such that
         (currpage <= x < endpage) on node home
     }
  */
}

void MyFree(void *block) {
  long home;

  home = HOME(block);
  {
    ;
    pthread_mutex_lock(&(mem_pool[home].memoryLock));
    ;
  }
  MyFreeNow(block);
  {
    ;
    pthread_mutex_unlock(&(mem_pool[home].memoryLock));
    ;
  }
}

void MyFreeNow(void *block) {
  long bucket, size, home;

  size = SIZE(block);
  home = HOME(block);

  if (size <= 0) {
    printf("Bad size %ld\n", size);
    exit(-1);
  }
  if (home < -1 || home >= mallocP) {
    printf("Bad home %ld\n", home);
    exit(-1);
  }

  if (size > MAXFASTBL)
    bucket = MAXFAST;
  else {
    bucket = FindBucket(size);
    if (size < 1 << bucket)
      bucket--;
  }

  /* throw away tiny blocks */
  if (bucket == 0)
    return;

  NEXTFREE(block) = (long *)mem_pool[home].freeBlock[bucket];
  mem_pool[home].freeBlock[bucket] = block;
  mem_pool[home].tally -= size;
}
