/* vi: set sw=4 ts=4: */
/*
 * $Id: ping.c,v 1.56 2004/03/15 08:28:48 andersen Exp $
 * Mini ping implementation for busybox
 *
 * Copyright (C) 1999 by Randolph Chung <tausq@debian.org>
 *
 * Adapted from the ping in netkit-base 0.10:
 * Copyright (c) 1989 The Regents of the University of California.
 * Derived from software contributed to Berkeley by Mike Muuss.
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>

#include <signal.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "tr069_ping.h"
#include "f_ping.h"
#include "tr069.h"

#include <n_lib.h>




static int datalen; /* intentionally uninitialized to work around gcc bug */
static int max_wait;
static long pingcount;

//FIXME:
//masked by alex .2011.7.19
int tr069_ping(const char *host, tr069_ping_argument *argument, tr069_ping_result *result)
{
	int ret = -1;

	tr069_debug("host:%s, size=%d, count=%d, timeout=%d\n",host,
		argument->data_size, argument->ping_count, argument->time_out);

	if (NULL == host || NULL == argument || NULL == result)
	{
		tr069_error("NULL parameters\n");
		return -1;
	}

	/* set arguments */
	datalen = argument->data_size;
	if (datalen <= 0 || datalen > 128)
	{
		tr069_error("invalid datalen\n");
		return -1;
	}

	pingcount = argument->ping_count;
	if (pingcount <= 0 || pingcount > 128)
	{
		tr069_error("invalid pingcount\n");
		return -1;
	}

	max_wait = argument->time_out / 1000;
	if (max_wait < 2 || max_wait > 120)
	{
		tr069_error("invalid max_wait\n");
		return -1;
	}

	tr069_debug("start ping %s\n", host);
	ret = f_ping(host, pingcount, max_wait, datalen, result);
	signal(SIGALRM, SIG_IGN);

	/* get result */
	return ret;
}
