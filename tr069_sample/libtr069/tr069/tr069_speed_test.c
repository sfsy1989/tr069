#include <curl/curl.h>
#include <dirent.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include<unistd.h>

#include "n_lib.h"
#include "tr069_speed_test.h"
#include "tr069.h"
#include "tr069_interface.h"
#include "tr069_service.h"

int g_tr069_speed_test_state = -1;
unsigned int g_tr069_speed_test_code;

double g_test_speed = 0;
extern int g_speedtest_flag ;
int speed_test_timerid = -1;
int g_max_time;


static int init_speed_test_env()
{
	DIR *dirp = NULL;
	char cmd[F_MAX_PATH_SIZE] = {0};

	dirp = opendir(IPTV_SPEED_TEST_DIR);
	if (NULL != dirp) {
		tr069_notice("testspeed dir exists\n");
		closedir(dirp);
		return FV_OK;
	}

	snprintf(cmd, sizeof(cmd), "mkdir -p %s",
		IPTV_SPEED_TEST_DIR);
	system(cmd);

	return FV_OK;
}

static void uninit_speed_test_env()
{
	DIR *dirp = NULL;
	char cmd[F_MAX_PATH_SIZE] = {0};

	dirp = opendir(IPTV_SPEED_TEST_DIR);
	if (NULL == dirp) {
		tr069_notice("testspeed dir not exist\n");
		return;
	}
	closedir(dirp);

	snprintf(cmd, sizeof(cmd), "rm -rf %s",
		IPTV_SPEED_TEST_DIR);
	system(cmd);
}

static void speed_test_download_cb(int result)
{
	tr069_notice("speed test finish result=%d\n", result);

	if (CURLE_OK == result) {
		g_tr069_speed_test_state = 0;//download finished
		g_tr069_speed_test_code = 200;
	}
	else {
		tr069_error("test speed download fail\n");
		g_tr069_speed_test_state = -1;
		g_tr069_speed_test_code = 301;
	}

	uninit_speed_test_env();
}


static int tr069_start_speed_test(char *filepath)
{
		if (g_tr069_speed_test_state != 1) {//
			/* check for net state */
			int net_state = 0;

			n_net_get_netwire_state(&net_state);
			if (net_state != 0) {
				tr069_error("net status=%d error\n", net_state);
				g_tr069_speed_test_code = 301;
				g_test_speed = 0;
				g_tr069_speed_test_state = -1;//exception
			}
			else {
				/* start speed test */
				g_tr069_speed_test_state = 1;
				g_tr069_speed_test_code = 200;
				if (FV_OK == init_speed_test_env()) {
				
					if (FV_OK != f_net_download_multi(IPTV_SPEED_TEST_FILE,
						filepath, 1,
						IPTV_SPEED_TEST_CTIMEOUT,
						IPTV_SPEED_TEST_DTIMEOUT,
						speed_test_download_cb)) 
					{
							g_tr069_speed_test_state = -1;
							g_tr069_speed_test_code = 301;
					}
				}
			}
		}
	
	return FV_OK;
}

static void tr069_get_speed_test_result(int junk)
{
	double test_speed;
	test_speed = f_net_get_last_download_speed() * 8.0 / 1024.0;
	if (g_tr069_speed_test_state != 0)
		g_test_speed = test_speed;
				
	tr069_debug("current=%.2f\n", g_test_speed);
}

int tr069_speed_test(struct speed_test_infos *info)
{

	g_max_time = info->duration - 1;
	tr069_debug("%s\n", info->filepath);
	tr069_start_speed_test(info->filepath);
	g_speedtest_flag = 1;
	
	speed_test_timerid = f_timer_add(1000, upload_speed_test, NULL);
	if (speed_test_timerid <= 0){
		tr069_error("f_timer_add error\n");
		return -1;
	}
	n_check_waitms();
	return 0;
}


int upload_speed_test(void *arg)
{
	static int count = 0;	
	INT8_T string_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	tr069_get_speed_test_result(0);

	tr069_get_config(TR069_PARAM_SpeedTestEnable, string_buff, sizeof(string_buff));
	if (!strncasecmp(string_buff, "false", sizeof("flase"))) {
		if (g_tr069_speed_test_state == 1) {
			/* stop speed test */
				g_tr069_speed_test_state = 0;
				f_net_stop_multi();
				uninit_speed_test_env();
		}	
		count = 0;
		speed_test_timerid = 0;
		g_speedtest_flag = 0;
		n_check_waitms();
		system("/ipstb/bin/fakeir portal");
		return -1;
	}
			
	if(count == g_max_time || g_tr069_speed_test_state != 1) {
		speed_test_timerid = 0;
		count = 0;
		g_speedtest_flag = 0;
		
		tr069_set_config(TR069_PARAM_SpeedTestEnable, "false");
		sprintf(string_buff, "%f", g_test_speed);
		tr069_set_config(TR069_PARAM_SpeedTestResult, string_buff);
		sprintf(string_buff, "%d", g_tr069_speed_test_code);
		tr069_set_config(TR069_PARAM_SpeedTestCode, string_buff);
		
		if (g_tr069_speed_test_state == 1) {
			/* stop speed test */
				g_tr069_speed_test_state = 0;
				f_net_stop_multi();
				uninit_speed_test_env();
				tr069_debug("stop downlaod\n");
		}	
		tr069_service_set_event(TR069_EVENT_DIAGNOSTICS_COMPLETE, 1, "");
		tr069_service_put_inform_parameter(PCODE_SpeedTestUniqueNum);
			//tr069_control_put_task(control_handle, TR069_TASK_SESSION);
		tr069_service_start();
		n_check_waitms();
		system("/ipstb/bin/fakeir portal");
		return -1;
	}
	count++;
	return 0;
}
