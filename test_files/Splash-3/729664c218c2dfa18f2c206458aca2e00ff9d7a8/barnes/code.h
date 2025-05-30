
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
 * CODE.H: define various global things for CODE.C.
 */

#include "code_io.h"
#include "defs.h"
#include "getparam.h"
#include "grav.h"
#include "load.h"
#include "stdinc_pre.h"
#include "util.h"
#include "vectmath.h"

#ifndef _CODE_H_
#define _CODE_H_

#define PAD_SIZE (PAGE_SIZE / (sizeof(long)))

/* Defined by the input file */
global string headline; /* message describing calculation */
global string infile;   /* file name for snapshot input */
global string outfile;  /* file name for snapshot output */
global real dtime;      /* timestep for leapfrog integrator */
global real dtout;      /* time between data outputs */
global real tstop;      /* time to stop calculation */
global long nbody;      /* number of bodies in system */
global real fcells;     /* ratio of cells/leaves allocated */
global real fleaves;    /* ratio of leaves/bodies allocated */
global real tol;        /* accuracy parameter: 0.0 => exact */
global real tolsq;      /* square of previous */
global real eps;        /* potential softening parameter */
global real epssq;      /* square of previous */
global real dthf;       /* half time step */
global long NPROC;      /* Number of Processors */

global long maxcell;    /* max number of cells allocated */
global long maxleaf;    /* max number of leaves allocated */
global long maxmybody;  /* max no. of bodies allocated per processor */
global long maxmycell;  /* max num. of cells to be allocated */
global long maxmyleaf;  /* max num. of leaves to be allocated */
global bodyptr bodytab; /* array size is exactly nbody bodies */

global struct CellLockType {
  pthread_mutex_t(CL)[MAXLOCK]; /* locks on the cells*/
} * CellLock;

long Global_n2bcalc;      /* total number of body/cell interactions  */
long Global_nbccalc;      /* total number of body/body interactions  */
long Global_selfint;      /* number of self interactions             */
real Global_mtot;         /* total mass of N-body system             */
real Global_etot[3];      /* binding, kinetic, potential energy      */
matrix Global_keten;      /* kinetic energy tensor                   */
matrix Global_peten;      /* potential energy tensor                 */
vector Global_cmphase[2]; /* center of mass coordinates and velocity */
vector Global_amvec;      /* angular momentum vector                 */
cellptr Global_G_root;    /* root of the whole tree                  */
vector Global_rmin;       /* lower-left corner of coordinate box     */
vector Global_min;        /* temporary lower-left corner of the box  */
vector Global_max;        /* temporary upper right corner of the box */
real Global_rsize;        /* side-length of integer coordinate box   */
pthread_mutex_t Global_Barstart_bar_mutex;
pthread_cond_t Global_Barstart_bar_cond;
unsigned Global_Barstart_bar_teller;
pthread_mutex_t Global_Bartree_bar_mutex;
pthread_cond_t Global_Bartree_bar_cond;
unsigned Global_Bartree_bar_teller;
pthread_mutex_t Global_Barcom_bar_mutex;
pthread_cond_t Global_Barcom_bar_cond;
unsigned Global_Barcom_bar_teller;
pthread_mutex_t Global_Barload_bar_mutex;
pthread_cond_t Global_Barload_bar_cond;
unsigned Global_Barload_bar_teller;
pthread_mutex_t Global_Baraccel_bar_mutex;
pthread_cond_t Global_Baraccel_bar_cond;
unsigned Global_Baraccel_bar_teller;
pthread_mutex_t Global_Barpos_bar_mutex;
pthread_cond_t Global_Barpos_bar_cond;
unsigned Global_Barpos_bar_teller;
pthread_mutex_t Global_CountLock; /* Lock on the shared variables            */
pthread_mutex_t
    Global_NcellLock; /* Lock on the counter of array of cells for loadtree */
pthread_mutex_t
    Global_NleafLock; /* Lock on the counter of array of leaves for loadtree */
pthread_mutex_t Global_io_lock;
unsigned long Global_createstart, Global_createend, Global_computestart,
    Global_computeend;
unsigned long Global_trackstart, Global_trackend, Global_tracktime;
unsigned long Global_partitionstart, Global_partitionend, Global_partitiontime;
unsigned long Global_treebuildstart, Global_treebuildend, Global_treebuildtime;
unsigned long Global_forcecalcstart, Global_forcecalcend, Global_forcecalctime;
long Global_current_id;

/* This structure is needed because under the sproc model there is no
 * per processor private address space.
 */
struct local_memory {
  /* Use padding so that each processor's variables are on their own page */
  long pad_begin[PAD_SIZE];

  real tnow;  /* current value of simulation time */
  real tout;  /* time next output is due */
  long nstep; /* number of integration steps so far */

  long workMin, workMax; /* interval of cost to be treated by a proc */

  vector min, max; /* min and max of coordinates for each Proc. */

  long mynumcell;     /* num. of cells used for this proc in ctab */
  long mynumleaf;     /* num. of leaves used for this proc in ctab */
  long mynbody;       /* num bodies allocated to the processor */
  bodyptr *mybodytab; /* array of bodies allocated / processor */
  long myncell;       /* num cells allocated to the processor */
  cellptr *mycelltab; /* array of cellptrs allocated to the processor */
  long mynleaf;       /* number of leaves allocated to the processor */
  leafptr *myleaftab; /* array of leafptrs allocated to the processor */
  cellptr ctab;       /* array of cells used for the tree. */
  leafptr ltab;       /* array of cells used for the tree. */

  long myn2bcalc; /* body-body force calculations for each processor */
  long mynbccalc; /* body-cell force calculations for each processor */
  long myselfint; /* count self-interactions for each processor */
  long myn2bterm; /* count body-body terms for a body */
  long mynbcterm; /* count body-cell terms for a body */
  bool skipself;  /* true if self-interaction skipped OK */
  bodyptr pskip;  /* body to skip in force evaluation */
  vector pos0;    /* point at which to evaluate field */
  real phi0;      /* computed potential at pos0 */
  vector acc0;    /* computed acceleration at pos0 */
  vector dr;      /* data to be shared */
  real drsq;      /* between gravsub and subdivp */
  nodeptr pmem;   /* remember particle data */

  nodeptr Current_Root;
  long Root_Coords[NDIM];

  real mymtot;         /* total mass of N-body system */
  real myetot[3];      /* binding, kinetic, potential energy */
  matrix myketen;      /* kinetic energy tensor */
  matrix mypeten;      /* potential energy tensor */
  vector mycmphase[2]; /* center of mass coordinates */
  vector myamvec;      /* angular momentum vector */

  long pad_end[PAD_SIZE];
};
global struct local_memory Local[MAX_PROC];

void SlaveStart(void);
void stepsystem(long ProcessId);
void ComputeForces(long ProcessId);
void Help(void);
void ANLinit(void);
void init_root(void);
void tab_init(void);
void startrun(void);
void testdata(void);
void pickshell(real vec[], real rad);
void find_my_initial_bodies(bodyptr btab, long nbody, long ProcessId);
void find_my_bodies(nodeptr mycell, long work, long direction, long ProcessId);
void Housekeep(long ProcessId);
void setbound(void);
long Log_base_2(long number);
long intpow(long i, long j);

#endif
