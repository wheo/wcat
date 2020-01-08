#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "misc.h"

#ifdef DEBUG
#undef DEBUG
#define _d(fmt, args...)     \
	{                        \
		printf(fmt, ##args); \
	}
#else
#undef DEBUG
#define _d(fmt, args...)
#endif

#define SAFE_DELETE(x) \
	{                  \
		if (x)         \
			delete x;  \
		x = NULL;      \
	}
#define MAX_NUM_NICS 5

typedef struct
{
	int state;
	int node;
	int nics;

	char ip[2][32];
	char nic[MAX_NUM_NICS][32];
} wcat_s;

extern wcat_s g;
#endif
