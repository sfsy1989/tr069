#include <n_lib.h>
#include "tr069_traceroute.h"


int tr069_tracert(const char *host, f_tracert_argument *argument, f_tracert_result *result)
{
	int ret;

	memset(result, 0, sizeof(result));
	ret = f_traceroute(host, argument, result);

	if (result->getdstip == 0) {
		memset(result, 0, sizeof(result));
		ret = f_tracert(host, argument, result);
	}

	return ret;
}



