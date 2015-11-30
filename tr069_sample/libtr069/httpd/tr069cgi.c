#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cgic.h"
#include "n_lib.h"

int cgiMain()
{
	int ret;

	if (FV_OK != n_lib_init("", NULL)) {
		return -1;
	}

	n_stb_stat_init();
	f_stb_status_s *stb_stat = n_get_stb_stat();
	snprintf(stb_stat->tr069_session, F_TR069_SESSION_ID_SIZE, "%s", cgiCookie);
	ret = n_tr069_set_event(TR069_EVENT_CONNECTION_REQUEST, 1, "");
	if (0 == ret) {
		n_tr069_put_inform_param(PCODE_STBID);
		n_tr069_put_inform_param(PCODE_IPAddressReal);
		n_tr069_start();
	}

	cgiHeaderContentType("text/plain");
	fprintf(cgiOut, "start tr069 session, cookie=%s\r\n", stb_stat->tr069_session);
	n_lib_uninit();
	return 0;
}
