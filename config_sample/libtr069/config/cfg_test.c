#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "n_lib.h"
//fixme
//#include "cfg_log_config.h"

int cfg_tst_fvipc_get(void *arg)
{
	int i = 0;
	char getbuf[256];

	for(i=0; i < F_CFG_NUM; i++) {
		memset(getbuf, 0x00, sizeof(getbuf));
		n_cfg_get_config(i,getbuf,sizeof(getbuf));
		printf("%d ========= %s\n", i, getbuf);
	}

	return FV_OK;
}

int cfg_tst_fvipc_get_nostop(void *arg)
{
	while(1) {
		int i = 0;
		char getbuf[4096];
		for(i=0; i < F_CFG_NUM; i++) {
			memset(getbuf, 0x00, sizeof(getbuf));
			n_cfg_get_config(i,getbuf,sizeof(getbuf));
		}
	}
	return FV_OK;
}

int cfg_tst_fvipc_set(void *arg)
{
	int i = 0;
	while (i++ < 1000) {
		n_cfg_set_config_int(F_CFG_MP_Volume, i % 100);
	}

	return FV_OK;
}

//fixme
#if 0
int cfg_tst_log_udp(void *arg)
{
	cfg_log_msg msg = {"1234567890abcde", "172.16.14.22", 0,0,0,{0},{0},{0},{0},{0},{0},
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,{0},{0},{0},{0},{0},{0},{0},{0},
		{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},0};
	cfg_log_config(&msg);
	printf("************msg.ip_addr is %s\n", msg.ip_addr);
	printf("************msg.mode is %d\n", msg.mode);
	return FV_OK;
}
#endif

int main(int argc, char **argv)
{
	n_lib_init("cfg_test", NULL);
	f_test_init(argc, argv);

	//fixme
	//f_test_add_data_func("/cfg/log/set", cfg_tst_log_udp, NULL);
	f_test_add_data_func("/cfg/fvipc/get", cfg_tst_fvipc_get, NULL);
	f_test_add_data_func("/cfg/fvipc/get_nostop", cfg_tst_fvipc_get_nostop, NULL);	
	f_test_add_data_func("/cfg/fvipc/set", cfg_tst_fvipc_set, NULL);

	f_test_run();
    	n_lib_uninit();

	return 0;
}

