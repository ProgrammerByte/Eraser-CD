

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

/******************************************************************************
 *                                                                             *
 *    map.c:   Loads 3D density map.                                           *
 *                                                                             *
 ******************************************************************************/

#include "incl.h"
#include <string.h>

/* The following declarations show the layout of the .den file.              */
/* If changed, the version number must be incremented and code               */
/* written to handle loading of both old and current versions.               */

/* Version for new .den files:               */
#define MAP_CUR_VERSION 1 /*   Initial release                         */
short map_version;        /* Version of this .den file                 */

short orig_min[NM], /* Dimensions of original data file          */
    orig_max[NM],   /*   (CT:  from <file>.header file           */
    orig_len[NM];   /*    ED:  from <file>.mi file)              */

short extr_min[NM], /* Portion of file extracted for this map    */
    extr_max[NM],   /*   (mins and maxes will be subset of       */
    extr_len[NM];   /*    orig and lengths will be <= orig)      */

short map_min[NM], /* Dimensions of this map                    */
    map_max[NM],   /*   (mins will be 0 in this program and     */
    map_len[NM];   /*    lens may be != extr if warps > 0)      */

short map_warps; /* Number of warps since extraction          */
                 /*   (0 = none)                              */

int map_length;       /* Total number of densities in map          */
                      /*   (= product of lens)                     */
DENSITY *map_address; /* Pointer to map                            */

/* End of layout of .den file.                                               */

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

void Load_Map(char filename[]) {
  char local_filename[FILENAME_STRING_SIZE];
  int fd;

  strcpy(local_filename, filename);
  strcat(local_filename, ".den");
  fd = Open_File(local_filename);

  Read_Shorts(fd, (unsigned char *)&map_version, (long)sizeof(map_version));
  if (map_version != MAP_CUR_VERSION)
    Error("    Can't load version %d file\n", map_version);

  Read_Shorts(fd, (unsigned char *)orig_min, (long)sizeof(orig_min));
  Read_Shorts(fd, (unsigned char *)orig_max, (long)sizeof(orig_max));
  Read_Shorts(fd, (unsigned char *)orig_len, (long)sizeof(orig_len));

  Read_Shorts(fd, (unsigned char *)extr_min, (long)sizeof(extr_min));
  Read_Shorts(fd, (unsigned char *)extr_max, (long)sizeof(extr_max));
  Read_Shorts(fd, (unsigned char *)extr_len, (long)sizeof(extr_len));

  Read_Shorts(fd, (unsigned char *)map_min, (long)sizeof(map_min));
  Read_Shorts(fd, (unsigned char *)map_max, (long)sizeof(map_max));
  Read_Shorts(fd, (unsigned char *)map_len, (long)sizeof(map_len));

  Read_Shorts(fd, (unsigned char *)&map_warps, (long)sizeof(map_warps));
  Read_Longs(fd, (unsigned char *)&map_length, (long)sizeof(map_length));

  Allocate_Map(&map_address, map_length);

  printf("    Loading map from .den file...\n");
  Read_Bytes(fd, (unsigned char *)map_address,
             (long)(map_length * sizeof(DENSITY)));
  Close_File(fd);
}

void Allocate_Map(DENSITY **address, long length) {
  long i;

  printf("    Allocating density map of %ld bytes...\n",
         length * sizeof(DENSITY));

  *address = (DENSITY *)malloc(length * sizeof(DENSITY));
  ;

  if (*address == NULL)
    Error("    No space available for map.\n");
  else
    for (i = 0; i < length; i++)
      *(*address + i) = 0;
}

void Deallocate_Map(DENSITY **address) {
  printf("    Deallocating density map...\n");

  /*  G_FREE(*address);  */

  *address = NULL;
}
