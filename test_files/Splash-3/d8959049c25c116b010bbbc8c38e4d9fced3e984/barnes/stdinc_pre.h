#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#ifndef _STDINC_PRE_H_
#define _STDINC_PRE_H_

#define global extern

#define error(msg, ...) printf(msg, ##__VA_ARGS__);
/*
 * STREAM: a replacement for FILE *.
 */

typedef FILE *stream;

/*
 * BOOL, TRUE and FALSE: standard names for logical values.
 */

typedef long bool;

#ifndef TRUE

#define FALSE 0
#define TRUE 1

#endif

/*
 * BYTE: a short name for a handy chunk of bits.
 */

typedef char byte;

/*
 * STRING: for null-terminated strings which are not taken apart.
 */

typedef char *string;

/*
 * REAL: default type is double;
 */

typedef double real, *realptr;

/*
 * PROC, IPROC, RPROC: pointers to procedures, integer functions, and
 * real-valued functions, respectively.
 */

typedef void (*proced)();
typedef long (*iproc)();
typedef real (*rproc)();

/*
 * LOCAL: declare something to be local to a file.
 * PERMANENT: declare something to be permanent data within a function.
 */

#define local static
#define permanent static

/*
 * STREQ: handy string-equality macro.
 */

#define streq(x, y) (strcmp((x), (y)) == 0)

/*
 *  PI, etc.  --  mathematical constants
 */

#define PI 3.14159265358979323846
#define TWO_PI 6.28318530717958647693
#define FOUR_PI 12.56637061435917295385
#define HALF_PI 1.57079632679489661923
#define FRTHRD_PI 4.18879020478639098462

/*
 *  ABS: returns the absolute value of its argument
 *  MAX: returns the argument with the highest value
 *  MIN: returns the argument with the lowest value
 */

#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif