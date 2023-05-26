#ifndef __LOCAL_H__
#define __LOCAL_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <string>
#include <limits.h>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/wait.h>

#define Q_SEED 'a'                  /* seed for ftok for message queue*/
#define MEM_SEED 'b'                /* seed for ftok for shared memory */
#define MEM_NUM_OF_READERS_SEED 'c' /* seed for ftok for num of readers shared memory */
#define SEM_MUT_SEED 'd'            /* seed for ftok for mutex semaphore */
#define SEM_R_SEED 'f'              /* seed for ftok for readers semaphore */
#define SEM_W_SEED 'g'              /* seed for ftok for writers semaphore */

// #define NUMBER_OF_COLUMNS 1000
#define MAX_STRING_LENGTH 1024

typedef struct
{
  long msg_to; /* Placed in the queue for */
               //   long      msg_fm;		/* Placed in the queue by  */
  char buffer[BUFSIZ];
} MESSAGE;

union semun
{
  int val;
  struct semid_ds *buf;
  ushort *array;
};

struct MEMORY
{
  int rows;
  // int cols;
  char data[][MAX_STRING_LENGTH]; // Flexible array member
};

struct NUM_OF_READERS
{
  // flexible array cannot be the only member of a struct
  unsigned numOfColumns;
  // number of readers of each column
  unsigned readers[]; // Flexible array member
};

#endif