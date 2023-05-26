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


#define SEED   'm'		/* seed for ftok */
// #define NUMBER_OF_COLUMNS 1000
//#define MAX_STRING_LENGTH 1024


typedef struct {
  long      msg_to;		/* Placed in the queue for */
//   long      msg_fm;		/* Placed in the queue by  */
  char      buffer[BUFSIZ];
} MESSAGE;

union semun {
  int              val;
  struct semid_ds *buf;
  ushort          *array; 
};

struct MEMORY
{
    int rows;
    int cols;
    char* data[]; // Flexible array member
};

// struct MEMORY {
//   int  head, tail;
//   char buffer[NUMBER_OF_COLUMNS][MAX_STRING_LENGTH];
  
// };


#endif