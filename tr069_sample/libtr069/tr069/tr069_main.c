/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: Main
	Function: Main entry point
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include <n_lib.h>
#include <n_stb_tr069.h>
#include "tr069.h"
#include "tr069_dump.h"
#include "tr069_dispatch.h"
#include "tr069_service.h"
#include "tr069_timer.h"

#include <utils/Log.h>

#define LOG_TAG "Tr069Service-JNI"

/* global */
dictionary *tr069_status_ini = NULL;
cpe_dump_s *cpe_dump = NULL;
int g_tr069_stand;
lan_statistics_s *net_stat = NULL;
brows_iptv_stat_s *brows_stat = NULL;
mplayer_iptv_statistics_s *mplayer_stat = NULL;

/* extern */

/* local */
typedef void (*void_func)(int);

/*dump cpe information using share memory*/
int init_cpe_dump()
{
	cpe_dump = (cpe_dump_s*)f_get_dump_addr(DP_CPE_DBG, sizeof(cpe_dump_s));
	f_assert(cpe_dump != NULL);
	if (cpe_dump == NULL) {
		return -1;
	} else {
		bzero(cpe_dump, sizeof(cpe_dump_s));
		cpe_dump->ver = CPE_VER;
		snprintf(cpe_dump->build_time, 128, CPE_BUILD_TIME);
		cpe_dump->call_flag = 0;
		cpe_dump->tr069_status = 0;
		cpe_dump->config_sem_value = -1;
		cpe_dump->session_sem_value = -1;
		return 0;
	}
}

void init_cpe_stat()
{
 	net_stat = (lan_statistics_s *) f_get_dump_addr(DP_FVNET_IPTV, 
		sizeof(lan_statistics_s));
	brows_stat = (brows_iptv_stat_s *) f_get_dump_addr(DP_FVBROWSER_IPTV, 
		sizeof(brows_iptv_stat_s));
	mplayer_stat = (mplayer_iptv_statistics_s *) f_get_dump_addr(DP_MPLAYER_IPTV, 
		sizeof(mplayer_iptv_statistics_s));

	if (net_stat == NULL)
		tr069_error("net stat unavailable\n");
	if (brows_stat == NULL)
		tr069_error("brows stat unavailable\n");
	if (mplayer_stat == NULL)
		tr069_error("mplayer stat unavailable\n");
}

void uninit_cpe_stat()
{
	if (net_stat != NULL) {
		f_dump_rel(net_stat, sizeof(lan_statistics_s));
		net_stat = NULL;
	}
	
	if (brows_stat != NULL) {
		f_dump_rel(brows_stat, sizeof(brows_iptv_stat_s));
		brows_stat = NULL;
	}

	if (mplayer_stat != NULL) {
		f_dump_rel(mplayer_stat, sizeof(mplayer_iptv_statistics_s));
		mplayer_stat = NULL;
	}
}

static void tr069_create_default_ini()
{
	FILE *config_file = NULL;

	config_file = fopen(TR069_STATUS_FILENAME, "w");
	f_assert(config_file);
	if (NULL == config_file) {
		tr069_error("create default config ini error\n");
		return;
	}

	fprintf(config_file,
	        "[Tr069]\n"
	        "FirstBoot = 1\n"
	        "Enable = 1\n"
	        "EventFlag = "
	        TR069_DEFAULT_EVENT_FLAG
	        "\n"
	        "RequestQueue = \n"
	        "InformParameters = \n"
	        "CommandKey = \n"
	        "TransferStartTime = \n"
	        "TransferCompleteTime = \n"
	        "TransferResult = 0\n"
	        "\n"
	        );

	fclose(config_file);
	return;
}

void tr069_save_status()
{
	char tmpfile_name[256] = {0};
	char cmd[256] = {0};
	int size;
	FILE * config_file = NULL;

	sprintf(tmpfile_name, "%s.tmp", TR069_STATUS_FILENAME);
	config_file = fopen(tmpfile_name, "w");
	f_assert(config_file);
	if(NULL == config_file) {
		tr069_error("create temp config ini error\n");
		return;
	}

	iniparser_dump_ini(tr069_status_ini, config_file);
	fseek(config_file, SEEK_SET, SEEK_END);
	size = ftell(config_file);
	fclose(config_file);
	if (size > 10) {
		/* overwrite original file */
		sprintf(cmd, "mv %s %s", tmpfile_name, TR069_STATUS_FILENAME);
		tr069_debug("cmd is %s\n", cmd);
		system(cmd);
	} else {
		/* remove error file */
		tr069_error("dump temp config ini error\n");
		unlink(tmpfile_name);
	}

	return;
}

int tr069_task()
{
	//sigset_t bset, oset;
	int ret;
	int step = 0;
	/* init flib */
	if (0 != n_lib_init("tr069", NULL)) {
		printf("lib_init failed\n");
		n_lib_uninit();
		goto ERROR_EXIT;
	}

	step++;

	//f_daemon_init();

	init_cpe_dump();
	init_cpe_stat();

	/* load tr069 ini */
	tr069_status_ini = iniparser_load(TR069_STATUS_FILENAME);
	f_assert(tr069_status_ini);
	if (NULL == tr069_status_ini) {
		/* tr069 ini error, create default */
		tr069_error("load tr069 ini error, create default ini\n");
		tr069_create_default_ini();

		tr069_status_ini = iniparser_load(TR069_STATUS_FILENAME);
		f_assert(tr069_status_ini);
		if (NULL == tr069_status_ini) {
			tr069_error("create default ini error\n");
			goto ERROR_EXIT;
		}
	}

	step++;

	if (0 != tr069_dispatch_init()) {		
		tr069_debug("tr069_dispatch_init error\n");
		goto ERROR_EXIT;
	}

	step++;

	signal (SIGQUIT, (void_func)tr069_service_stop);

	ret = n_iptv_standard();
	switch(ret) {
		case N_IPTVS_CU:
			g_tr069_stand = TR069_STAND_CU;
			break;
		case N_IPTVS_CTC:
			g_tr069_stand = TR069_STAND_CTC;
			break;
		default:
			g_tr069_stand = TR069_STAND_CTC;
			break;
	}
        
	n_mod_set_stat(N_MOD_RUNNING);
	ret = tr069_main();
	if (FV_OK != ret) {
		tr069_error("start tr069 main proc failed.\n");
		goto ERROR_EXIT;

	}

	//f_daemon_start();

	tr069_dispatch_start();
	/* write dict back to tr069 status ini and tr069 ini */
	tr069_debug("tr069 save config\n");
	tr069_save_status();
	n_cfg_save_tr069();
	iniparser_freedict(tr069_status_ini);
	uninit_cpe_stat();
	n_lib_uninit();

	return FV_OK;


ERROR_EXIT:

	switch (step) {
	case 3:
		tr069_dispatch_uninit();
	case 2:
		iniparser_freedict(tr069_status_ini);
	case 1:
		n_lib_uninit();
	default:
		break;
	}

	return 0;
}
