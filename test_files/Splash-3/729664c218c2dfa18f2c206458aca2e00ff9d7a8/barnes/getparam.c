
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

/*
 * GETPARAM.C:
 */

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#if __STDC_VERSION__ >= 201112L
#include <stdatomic.h>
#endif
#include <stdint.h>
#define PAGE_SIZE 4096
#define __MAX_THREADS__ 256

extern pthread_t __tid__[__MAX_THREADS__];
extern unsigned __threads__;
extern pthread_mutex_t __intern__;

#include "stdinc.h"

local string *defaults = NULL; /* vector of "name=value" strings */

/*
 * INITPARAM: ignore arg vector, remember defaults.
 */

void initparam(string *defv) { defaults = defv; }

/*
 * GETPARAM: export version prompts user for value.
 */

string getparam(string name) {
  long i, leng;
  string def;
  char buf[128];

  if (defaults == NULL)
    error("getparam: called before initparam\n");
  i = scanbind(defaults, name);
  if (i < 0)
    error("getparam: %s unknown\n", name);
  def = extrvalue(defaults[i]);
  fgets(buf, 128, stdin);
  leng = strlen(buf) + 1;
  buf[leng - 1] = '\0';
  leng--;
  if (leng > 1) {
    return (strcpy(malloc(leng), buf));
  } else {
    return (def);
  }
}

/*
 * GETIPARAM, ..., GETDPARAM: get long, long, bool, or double parameters.
 */

long getiparam(string name) {
  string val;

  for (val = ""; *val == '\0';) {
    val = getparam(name);
  }
  return (atoi(val));
}

long getlparam(string name) {
  string val;

  for (val = ""; *val == '\0';)
    val = getparam(name);
  return (atol(val));
}

bool getbparam(string name) {
  string val;

  for (val = ""; *val == '\0';)
    val = getparam(name);
  if (strchr("tTyY1", *val) != NULL) {
    return (TRUE);
  }
  if (strchr("fFnN0", *val) != NULL) {
    return (FALSE);
  }
  error("getbparam: %s=%s not bool\n", name, val);
}

double getdparam(string name) {
  string val;

  for (val = ""; *val == '\0';) {
    val = getparam(name);
  }
  return (atof(val));
}

/*
 * SCANBIND: scan binding vector for name, return index.
 */

long scanbind(string bvec[], string name) {
  long i;

  for (i = 0; bvec[i] != NULL; i++)
    if (matchname(bvec[i], name))
      return (i);
  return (-1);
}

/*
 * MATCHNAME: determine if "name=value" matches "name".
 */

bool matchname(string bind, string name) {
  char *bp, *np;

  bp = bind;
  np = name;
  while (*bp == *np) {
    bp++;
    np++;
  }
  return (*bp == '=' && *np == '\0');
}

/*
 * EXTRVALUE: extract value from name=value string.
 */

string extrvalue(string arg) {
  char *ap;

  ap = (char *)arg;
  while (*ap != '\0')
    if (*ap++ == '=')
      return ((string)ap);
  return (NULL);
}
