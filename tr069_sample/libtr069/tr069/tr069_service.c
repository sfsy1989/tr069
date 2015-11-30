/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: Service
	Function: Service interfaces(internal)
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <n_lib.h>
#include "tr069_service.h"
#include "tr069_control.h"
#include "tr069_dump.h"
#include "tr069_dispatch.h"
#include "cu_usb_config.h"
#include "tr069.h"

/* global */

/* extern */
extern int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
extern int tr069_set_config(int key, char *value);
extern int tr069_get_config(int key, char *value, unsigned int size);
extern int iniparser_set(dictionary *tr069_status_ini, char *key, char *value);

extern tr069_mgr *g_tr069;
extern cpe_dump_s *cpe_dump;
extern dictionary *tr069_status_ini;

/* local */
//static INT32_T tr069_on_reset(VOID_T *param);
static VOID_T tr069_service_wait_for_tr069();
static INT32_T tr069_service_target_to_set(tr069_mgr *tr069_handle);

static VOID_T tr069_service_wait_for_tr069()
{
	INT32_T wait_time = 0;
	const INT32_T sleep_interval = 200000;

	while (wait_time < TR069_MAX_WAIT_INIT_TIME_US) {
		if (g_tr069 != NULL) {
			break;
		} else {
			usleep(sleep_interval);
			wait_time += sleep_interval;
		}
	}

	return;
}

INT32_T tr069_service_start()
{
	tr069_control_mgr *control_handle = NULL;

	tr069_service_wait_for_tr069();
	f_assert(g_tr069);
	if (NULL == g_tr069 || NULL == g_tr069->control_handle) {
		tr069_error("tr069 handle error\n");
		return -1;
	}
	control_handle = g_tr069->control_handle;

	if (TR069_STATUS_WORKING == g_tr069->status) {
		tr069_debug("put task : TASK_SESSION\n");
		tr069_control_put_task(g_tr069->control_handle, TR069_TASK_SESSION);
	}

	return 0;
}

INT32_T tr069_service_stop ()
{
	tr069_control_mgr *control_handle;

	tr069_service_wait_for_tr069();
	f_assert(g_tr069);
	if (NULL == g_tr069) {
		tr069_error("tr069 handle error\n");
		return -1;
	}

	control_handle = g_tr069->control_handle;
	g_tr069->call_flag = 0;
	cpe_dump->call_flag = g_tr069->call_flag;

	if (TR069_STATUS_BEFORE_WORK == g_tr069->status) {
		tr069_debug("set status: AFTER_WORK\n");
		g_tr069->status = TR069_STATUS_AFTER_WORK;
		cpe_dump->tr069_status = g_tr069->status;
	} else if (TR069_STATUS_WORKING == g_tr069->status) {
		tr069_debug("set control finish flag\n");
		control_handle->finish_flag = 1;
	}
	tr069_dispatch_stop();

	return 0;
}

static INT32_T tr069_service_target_to_set(tr069_mgr *tr069_handle)
{
	tr069_control_mgr *control_handle;

	f_assert(tr069_handle);
	if (NULL == tr069_handle || NULL == tr069_handle->control_handle) {
		return -1;
	}
	control_handle = tr069_handle->control_handle;

	/* -1 for can not set, 0 for set config, 1 for set memory */
	switch (tr069_handle->status) {
	case TR069_STATUS_BEFORE_WORK:
	case TR069_STATUS_LOAD_ENV:
	case TR069_STATUS_AFTER_WORK:
		return 0;

	case TR069_STATUS_WORKING:
		switch (control_handle->status) {
		case TR069_CONTROL_STATUS_IDLE:
		case TR069_CONTROL_STATUS_TASK:
			return 1;

		case TR069_CONTROL_STATUS_DUMP:
		case TR069_CONTROL_STATUS_STOP:
		case TR069_CONTROL_STATUS_QUIT:
			return 0;

		default:
			return -1;
		}

	default:
		return -1;
	}
}

#if 0
static INT32_T tr069_on_reset(void *param)
{
	tr069_control_mgr *control_handle = NULL;

	tr069_debug( "reset timer triggered(pid: %d)\n", getpid());
	tr069_service_wait_for_tr069();
	f_assert(g_tr069);
	if (NULL == g_tr069 || NULL == g_tr069->control_handle) {
		f_error(N_LOG_CPE, "tr069 handle error\n");
		return 0;
	}
	control_handle = g_tr069->control_handle;

	/*
	if(-1 != reset_timer)
	{
		g_source_remove(reset_timer);
		reset_timer = -1;
	}
	*/

	tr069_debug( "start reset\n");
	if (g_tr069->call_flag > 0) {
		/* session sem off
		sem_wait(&control_handle->session_sem);
		sem_getvalue(&control_handle->session_sem, &cpe_dump->session_sem_value);
		*/

		if (TR069_STATUS_BEFORE_WORK == g_tr069->status) {
			tr069_debug( "set status : LOAD_ENV\n");
			g_tr069->status = TR069_STATUS_LOAD_ENV;
			cpe_dump->tr069_status = g_tr069->status;
		} else if (TR069_STATUS_WORKING == g_tr069->status) {
			tr069_debug( "put task : TASK_SESSION\n");
			tr069_control_put_task(g_tr069->control_handle, TR069_TASK_SESSION);
		}
	}
	g_tr069->call_flag = 0;
	cpe_dump->call_flag = g_tr069->call_flag;
	//sem_post(&control_handle->session_sem);

	return 1;
}
#endif

INT32_T tr069_service_set_event (int event_index, char event_value, char *command_key)
{
	INT32_T ret = -1;
	tr069_control_mgr *control_handle = NULL;
	INT8_T event_buff[TR069_EVENT_NUMBER] = {0};
	struct timespec time_out;

	time_out.tv_sec = time(NULL) + 1;
	time_out.tv_nsec = 0;

	f_assert(event_index >= 0 && event_index < TR069_EVENT_NUMBER);
	if (event_index < 0 || event_index >= TR069_EVENT_NUMBER) {
		tr069_error("event index(%d) error\n", event_index);
		return -1;
	}

	f_assert(0 == event_value || 1 == event_value);
	if (0 != event_value && 1 != event_value) {
		tr069_error("event value(%d) error\n", event_value);
		return -1;
	}

	tr069_service_wait_for_tr069();
	f_assert(g_tr069);
	if (NULL == g_tr069 || NULL == g_tr069->control_handle) {
		tr069_error("tr069 handle error\n");
		return -1;
	}

	control_handle = g_tr069->control_handle;
	/* check for sempahore, whether there is other thread using tr069 now */
	if (g_tr069->call_flag >= TR069_MAX_TASK_NUM) {
		tr069_error("task queue full\n");
		return -1;
	}

	switch (tr069_service_target_to_set(g_tr069)) {
	case 0:
		strncpy(event_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_EVENTFLAG, TR069_DEFAULT_EVENT_FLAG), sizeof(event_buff) - 1);
		ret = 0;
		if (ret != 0) {
			tr069_error("load event flag error\n");
			break;
		}

		/* then set and save to config */
		if (event_index >= TR069_EVENT_BOOTSTRAP && event_index < TR069_EVENT_NUMBER) {
			event_buff[event_index] = (1 == event_value ? '1' : '0');
			ret = iniparser_set(tr069_status_ini, TR069_STATUS_EVENTFLAG, event_buff);
			if (ret != 0) {
				tr069_error("set config error\n");
				break;
			}
		} else {
			ret = -1;
			break;
		}

		/* set command key */
		if (command_key != NULL && *command_key == 0) {
			ret = 0;
		}
		else if (command_key != NULL && strlen(command_key) < 32) {
			ret = iniparser_set(tr069_status_ini, TR069_STATUS_CMDKEY, command_key);
		}
		else {
			ret = -1;
		}

		break;

	case 1:
		/* set memory */
		if (*command_key != 0) {
			ret = tr069_control_set_event(control_handle, event_index, event_value, command_key);
		}
		else {
			ret = tr069_control_set_event(control_handle, event_index, event_value, NULL);
		}
		break;

	default:
		/* can not set */
		break;
	}

	if (0 == ret) {
		if (event_index < TR069_EVENT_M_SCHEDULEINFORM) {
			/* primary event, add call_flag */
			g_tr069->call_flag++;
		}
		cpe_dump->call_flag = g_tr069->call_flag;
	}
	return ret;
}

INT32_T tr069_service_put_request (int request)
{
	INT32_T ret = -1;
	tr069_control_mgr *control_handle = NULL;
	INT8_T request_buff[TR069_MAX_REQUEST_NUM] = {0};
	INT8_T request_char = '0';

	f_assert(request > TR069_RPC_ACS_INFORM && request <= TR069_RPC_ACS_TRANSFER_COMPLETE);
	if (request <= TR069_RPC_ACS_INFORM || request > TR069_RPC_ACS_TRANSFER_COMPLETE) {
		tr069_error("invalid request(%d)\n", request);
		return -1;
	}

	tr069_service_wait_for_tr069();
	f_assert(g_tr069);
	if (NULL == g_tr069 || NULL == g_tr069->control_handle) {
		tr069_error("tr069 handle error\n");
		return -1;
	}

	control_handle = g_tr069->control_handle;

	switch (tr069_service_target_to_set(g_tr069)) {
	case 0:
		/* set config */
//		sem_wait(&g_tr069->config_sem);
//		sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);

		/* first load current value from config */
		//*ret = tr069_get_config(TR069_STATUS_REQ_QUEUE, request_buff, sizeof(request_buff));
		strncpy(request_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_REQ_QUEUE, ""), sizeof(request_buff) - 1);
		ret = 0;
		if (ret != 0) {
			tr069_error("load request queue error\n");
//			sem_post(&g_tr069->config_sem);
//			sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);
			break;
		}

		/* then set and save to config */
		if ((strlen(request_buff) + 1) < TR069_MAX_REQUEST_NUM) {
			/* todo: add more supported request */
			if (TR069_RPC_ACS_TRANSFER_COMPLETE == request) {
				request_char = '2';
			} else {
				//f_assert_not_reached();
				ret = -1;
//				sem_post(&g_tr069->config_sem);
//				sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);
				break;
			}
			request_buff[strlen(request_buff) + 1] = 0;
			request_buff[strlen(request_buff)] = request_char;

			//*ret = tr069_set_config(TR069_STATUS_REQ_QUEUE, request_buff);
			ret = iniparser_set(tr069_status_ini, TR069_STATUS_REQ_QUEUE, request_buff);
		} else {
			ret = -1;
//			sem_post(&g_tr069->config_sem);
//			sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);
			break;
		}
//		sem_post(&g_tr069->config_sem);
//		sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);
		break;

	case 1:
		/* set memory */
		ret = tr069_control_put_request(control_handle, request);
		break;

	default:
		/* can not set */
		break;
	}

	return ret;
}

INT32_T tr069_service_put_inform_parameter (char *param_name)
{
	INT32_T ret = -1;
	tr069_control_mgr *control_handle = NULL;
	INT8_T string_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};

	f_assert(param_name);
	if (NULL == param_name) {
		tr069_error("parameter name error\n");
		return -1;
	}

	tr069_service_wait_for_tr069();
	f_assert(g_tr069);
	if (NULL == g_tr069 || NULL == g_tr069->control_handle) {
		tr069_error("tr069 handle error\n");
		return -1;
	}

	control_handle = g_tr069->control_handle;

	switch (tr069_service_target_to_set(g_tr069)) {
	case 0:
		/* set config */
//		sem_wait(&g_tr069->config_sem);
//		sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);

		/* first load current value from config */
		//*ret = tr069_get_config(TR069_STATUS_INFORM_PARAMS, string_buff, sizeof(string_buff));
	    strncpy(string_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_INFORM_PARAMS, ""), sizeof(string_buff) - 1);
		ret = 0;
		if (ret != 0) {
			tr069_error("load request queue error\n");
//			sem_post(&g_tr069->config_sem);
//			sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);
			break;
		}

		/* then set and save to config */
		if ((strlen(string_buff) + strlen(param_name) + 1) < sizeof(string_buff) && strlen(param_name) > 0) {
#if 0
			string_buff[strlen(string_buff) + strlen(param_name) + 1] = 0;
			if (string_buff[0] != 0)
				sprintf(string_buff + strlen(string_buff), ",%s", param_name);
			else
				sprintf(string_buff + strlen(string_buff), "%s", param_name);
			//*ret = tr069_set_config(TR069_STATUS_INFORM_PARAMS, string_buff);
			ret = iniparser_set(tr069_status, TR069_STATUS_INFORM_PARAMS, string_buff);
#else
			string_buff[strlen(string_buff) + strlen(param_name) + 1] = 0;
			sprintf(string_buff + strlen(string_buff), "%s", param_name);
			//*ret = tr069_set_config(TR069_STATUS_INFORM_PARAMS, string_buff);
			ret = iniparser_set(tr069_status_ini, TR069_STATUS_INFORM_PARAMS, string_buff);
#endif
		} else {
			tr069_error("put size error\n");
			ret = -1;
//			sem_post(&g_tr069->config_sem);
//			sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);
			break;
		}
//		sem_post(&g_tr069->config_sem);
//		sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);
		break;

	case 1:
		/* set memory */
		ret = tr069_control_put_inform_parameter(control_handle, param_name);
		break;

	default:
		/* can not set */
		break;
	}

	return ret;
}

INT32_T tr069_service_get_status (char *name, char **value)
{
	tr069_debug("get config %s\n", name);
	*value = strdup(iniparser_getstring(tr069_status_ini, name, ""));
	tr069_debug("value got is %s\n", *value);
	//strncpy(*value, iniparser_getstring(tr069_status, name, ""), size - 1);
	return 0;
}

INT32_T tr069_service_sync_status(char *name, char *value)
{
	char tmpfile_name[256] = {0};
	char cmd[256] = {0};
	int size;
	int ret;
	FILE * f;
	dictionary *temp_ini = NULL;

	temp_ini = iniparser_load(TR069_STATUS_FILENAME);
	if(NULL == temp_ini) {
		tr069_error("open the config file error!\n");
		return -1;
	}
	ret = iniparser_set(temp_ini, name, value);
	if (ret != 0) {
		iniparser_freedict(temp_ini);
		return -1;
	}

	sprintf(tmpfile_name, "%s.new", TR069_STATUS_FILENAME);
	f = fopen(tmpfile_name, "w");
	if(NULL == f) {
		tr069_error("open the save ini error!\n");
		iniparser_freedict(temp_ini);
		return -1;
	}

	iniparser_dump_ini(temp_ini, f);
	/* add by ash */
	fseek(f, SEEK_SET, SEEK_END);
	size = ftell(f);
	fclose(f);
	if (size > 10) {
		/* overwrite original file */
		sprintf(cmd, "mv %s %s", tmpfile_name, TR069_STATUS_FILENAME);
		system(cmd);
		ret = FV_OK;
	} else {
		/* remove error file */
		unlink(tmpfile_name);
		ret = -1;
	}

	iniparser_freedict(temp_ini);
	return ret;
}

INT32_T tr069_service_set_status (char *name, char *value)
{
	INT32_T ret = -1;
	tr069_debug("set config %s to %s\n", name, value);
	ret = iniparser_set(tr069_status_ini, name, value);

	/* if we set TR069_STATUS_FIRSTBOOT, save the setting to tr069.ini immediately */
	if (FV_OK == ret && strcmp(name, TR069_STATUS_FIRSTBOOT) == 0) {
		tr069_debug("set FirstBoot\n");
		ret = tr069_service_sync_status(name, value);
	}
	return ret;
}

INT32_T tr069_service_set_transfer_complete (char *start_time, char *complete_time, int transfer_result)
{
	INT32_T ret = -1;
	tr069_control_mgr *control_handle = NULL;
	INT8_T string_buff[16] = {0};

	tr069_service_wait_for_tr069();
	f_assert(g_tr069);
	if (NULL == g_tr069 || NULL == g_tr069->control_handle) {
		tr069_error("tr069 handle error\n");
		return -1;
	}

	control_handle = g_tr069->control_handle;

	switch (tr069_service_target_to_set(g_tr069)) {
	case 0:
		/* set config */
//		sem_wait(&g_tr069->config_sem);
//		sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);

		if (start_time != NULL && strlen(start_time) < 32) {
			//*ret = tr069_set_config(TR069_STATUS_TRANSTIME, start_time);
			ret = iniparser_set(tr069_status_ini, TR069_STATUS_TRANSTIME, start_time);
			if (ret != 0) {
//				sem_post(&g_tr069->config_sem);
//				sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);
				break;
			}
		}

		if (complete_time != NULL && strlen(complete_time) < 32) {
			//*ret = tr069_set_config(TR069_STATUS_TRANS_COMPLETE_TIME, complete_time);
			ret = iniparser_set(tr069_status_ini, TR069_STATUS_TRANS_COMPLETE_TIME, complete_time);
			if (ret != 0) {
//				sem_post(&g_tr069->config_sem);
//				sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);
				break;
			}
		}

		sprintf(string_buff, "%d", transfer_result);
		//*ret = tr069_set_config(TR069_STATUS_TRANS_RET, string_buff);
		ret = iniparser_set(tr069_status_ini, TR069_STATUS_TRANS_RET, string_buff);
		if (ret != 0) {
//			sem_post(&g_tr069->config_sem);
//			sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);
			break;
		}
//		sem_post(&g_tr069->config_sem);
//		sem_getvalue(&g_tr069->config_sem, &cpe_dump->config_sem_value);
		break;

	case 1:
		/* set memory */
		if (start_time != NULL) {
			strncpy(control_handle->transfer_stime, start_time, sizeof(control_handle->transfer_stime) - 1);
		}

		if (complete_time != NULL) {
			strncpy(control_handle->transfer_ctime, complete_time, sizeof(control_handle->transfer_ctime) - 1);
		}

		control_handle->transfer_result = transfer_result;

		ret = 0;
		break;

	default:
		/* can not set */
		break;
	}

	return ret;
}

INT32_T tr069_service_cu_usb_config()
{
	int ret;

	ret = cu_usb_config();
	if (ret == 0) {
		tr069_notice("cu usb config ok\n");
		n_module_stop(N_MOD_BROWS, 9);
		n_module_stop(N_MOD_MPLAYER, 9);
		n_osd_restart();
		n_osd_show_prompt(OSD_OP_SHOW, N_OSD_PT_MODIFY_PARAS);
	} else if (ret == -1) {
		tr069_error("cu usb config file open error\n");
		/* show open error osd */
	} else if (ret == -2) {
		tr069_error("cu usb config file parse error\n");
		/* show parse error osd */
	} else {
		tr069_error("cu usb config return unexpected\n");
	}

	return ret;
}
