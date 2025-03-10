

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

/*************************************************************************
 *                                                                        *
 *     anl.H:  ANL macros-related stuff, file should be included at end   *
 *              of static definitions section before function definitions *
 *                                                                        *
 **************************************************************************/

#include "const.h"
#include <pthread.h>
#define PAD 256

volatile long Global_Index;
volatile long Global_Queue[MAX_NUMPROC + 1][PAD];
pthread_barrier_t Global_SlaveBarrier;
pthread_barrier_t Global_TimeBarrier;
pthread_mutex_t Global_IndexLock;
pthread_mutex_t Global_CountLock;
pthread_mutex_t(Global_QLock)[MAX_NUMPROC + 1];
