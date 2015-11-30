#ifndef _tr069_ping_
#define _tr069_ping_
#include <f_ping.h>

/* argument pass to ping */
typedef struct _tr069_ping_argument
{
	/* number of repetitions */
	int ping_count;
	/* time out */
	int time_out;
	/* data block size */
	int data_size;
}tr069_ping_argument; 

int tr069_ping(const char *host, tr069_ping_argument *argument, tr069_ping_result *result);

#endif
