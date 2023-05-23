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

typedef struct {
  long      msg_to;		/* Placed in the queue for */
//   long      msg_fm;		/* Placed in the queue by  */
  char      buffer[BUFSIZ];
} MESSAGE;

#endif