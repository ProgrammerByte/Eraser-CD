

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
extern _Atomic __thread int __local_sense__;

#define global extern

#include "stdinc.h"

#define HZ 60.0
#define MULT 1103515245
#define ADD 12345
#define MASK (0x7FFFFFFF)
#define TWOTO31 2147483648.0

local long A = 1;
local long B = 0;
local long randx = 1;
local long lastrand; /* the last random number */

/*
 * XRAND: generate floating-point random number.
 */

double xrand(double xl, double xh) { return (xl + (xh - xl) * prand()); }

void pranset(long seed) {
  A = 1;
  B = 0;
  randx = (A * seed + B) & MASK;
  A = (MULT * A) & MASK;
  B = (MULT * B + ADD) & MASK;
}

double prand()
/*
        Return a random double in [0, 1.0)
*/
{
  lastrand = randx;
  randx = (A * randx + B) & MASK;
  return ((double)lastrand / TWOTO31);
}

/*
 * CPUTIME: compute CPU time in min.
 */
double cputime() {
  struct tms buffer;

  if (times(&buffer) == (clock_t)-1)
    error("times() call failed\n");
  return (buffer.tms_utime / (60.0 * HZ));
}
