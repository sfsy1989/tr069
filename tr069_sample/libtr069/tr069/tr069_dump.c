#include <stdio.h>
#include "tr069_dump.h"
#include <n_lib.h>
int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	cpe_dump_s *cpe_dump;

	cpe_dump = (cpe_dump_s*)f_get_dump_addr(DP_CPE_DBG, sizeof(cpe_dump_s));
	f_assert(cpe_dump);

	printf("cpe runtime status:\n");
	printf("--general status--\n");
	printf("version: %d\n", cpe_dump->ver);
	printf("version: %s\n", cpe_dump->build_time);
	printf("--tr069 status--\n");
	printf("call flag: %d\n", cpe_dump->call_flag);
	printf("tr069 status: %d\n", cpe_dump->tr069_status);
	printf("config sem value: %d\n", cpe_dump->config_sem_value);
	printf("--task status--\n");
	printf("session sem value: %d\n", cpe_dump->session_sem_value);

	return 0;
}
