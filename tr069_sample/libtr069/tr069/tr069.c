/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: Tr069
	Function: Public definition and model entry
********************************************************************/
#include <pthread.h>
#include <n_lib.h>
#include <n_stb_tr069.h>
#include "tr069.h"
#include "tr069_control.h"
#include "tr069_session.h"
#include "tr069_interface.h"
#include "tr069_rpc.h"
#include "tr069_parameter.h"
#include "tr069_dump.h"
#include "tr069_dispatch.h"
#include "tr069_service.h"

/* extern global */
extern int g_tr069_timer;
extern int g_tr111_timer;

/* global */
tr069_mgr *g_tr069 = NULL;

/* extern */
extern int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
extern int iniparser_set(dictionary *tr069_status_ini, char *key, char *value);
extern void tr069_save_status();

extern cpe_dump_s *cpe_dump;
extern dictionary *tr069_status_ini;

/* local */
INT32_T tr069_main()
{
	INT32_T ret;

	/* initialize */
	g_tr069 = tr069_init();
	f_assert(g_tr069);
	if (NULL == g_tr069) {
		tr069_error("initialize error\n");
		goto ERROR_EXIT;
	}

	/* load environment settings from ini file and enter main task */
	g_tr069->status = TR069_STATUS_LOAD_ENV;
	cpe_dump->tr069_status = g_tr069->status;
	ret = tr069_load_environment(g_tr069);
	if (-1 == ret) {
		tr069_error("load environment error\n");
		goto ERROR_EXIT;
	}

	g_tr069->status = TR069_STATUS_WORKING;
	cpe_dump->tr069_status = g_tr069->status;
	ret = tr069_control_main_task(g_tr069->control_handle);
	if (FV_OK != ret) {
		tr069_error("tr069_control_main_task failed.\n");
		goto ERROR_EXIT;
	}

    ret = tr111_control_main_task(g_tr069->tr111_handle);
	if (FV_OK != ret) {
		tr069_error("tr111_control_main_task failed.\n");
		goto ERROR_EXIT;
	}

	return FV_OK;

ERROR_EXIT:
	/* if tr069 manager already initialized, uninitialize it */
	if (NULL != g_tr069) {
		g_tr069 = tr069_uninit(g_tr069);
		tr069_error("error quit\n");
	}

	if (g_tr069_timer) {
		f_timer_remove(g_tr069_timer);
	}

	if (g_tr111_timer) {
		f_timer_remove(g_tr111_timer);
	}

	return -1;
}

tr069_mgr* tr069_init()
{
	tr069_mgr *manager = NULL;

	/* initialize tr069 manager */
	manager = malloc(sizeof(tr069_mgr));
	f_assert(manager);
	if (NULL == manager) {
		tr069_error("malloc tr069 manager error\n");
		goto ERROR_EXIT;
	}
	memset(manager, 0, sizeof(tr069_mgr));

	manager->control_handle = NULL;
	manager->tr111_handle = NULL;
	manager->interface_handle = NULL;
	manager->parameter_handle = NULL;
	manager->rpc_handle = NULL;
	manager->session_handle = NULL;

	/* initialize all elements */
	manager->control_handle = tr069_control_init(manager);
	f_assert(manager->control_handle);
	if (NULL == manager->control_handle) {
		tr069_error("initialize control manager error\n");
		goto ERROR_EXIT;
	}

	manager->tr111_handle = tr111_control_init(manager);
	f_assert(manager->tr111_handle);
	if (NULL == manager->tr111_handle) {
		tr069_error("initialize tr111 manager error\n");
		goto ERROR_EXIT;
	}

	manager->parameter_handle = tr069_parameter_init(manager);
	f_assert(manager->parameter_handle);
	if (NULL == manager->parameter_handle) {
		tr069_error("initialize parameter manager error\n");
		goto ERROR_EXIT;
	}

	manager->interface_handle = tr069_interface_init(manager);
	f_assert(manager->interface_handle);
	if (NULL == manager->interface_handle) {
		tr069_error("initialize interface manager error\n");
		goto ERROR_EXIT;
	}

	manager->rpc_handle = tr069_rpc_init(manager);
	f_assert(manager->rpc_handle);
	if (NULL == manager->rpc_handle) {
		tr069_error("initialize rpc manager error\n");
		goto ERROR_EXIT;
	}

	manager->session_handle = tr069_session_init(manager);
	f_assert(manager->session_handle);
	if (NULL == manager->session_handle) {
		tr069_error("initialize session manager error\n");
		goto ERROR_EXIT;
	}

	return manager;

ERROR_EXIT:

	/* if tr069 manager already initialized, uninitialize it */
	if (NULL != manager) {
		manager = tr069_uninit(manager);
	}

	tr069_error("error quit\n");
	return NULL;
}

tr069_mgr* tr069_uninit(tr069_mgr *tr069_handle)
{
//	INT32_T ret;
//	INT32_T sem_value;
	struct timespec time_out;

	time_out.tv_sec = time(NULL) + 1;
	time_out.tv_nsec = 0;

	f_assert(tr069_handle);
	tr069_debug("enter\n");
	if (NULL != tr069_handle) {
		/* uninitialize all elements */
		if (NULL != tr069_handle->session_handle) {
			tr069_handle->session_handle = tr069_session_uninit(tr069_handle->session_handle);
		}

		if (NULL != tr069_handle->rpc_handle) {
			tr069_handle->rpc_handle = tr069_rpc_uninit(tr069_handle->rpc_handle);
		}

		if (NULL != tr069_handle->interface_handle) {
			tr069_handle->interface_handle = tr069_interface_uninit(tr069_handle->interface_handle);
		}

		if (NULL != tr069_handle->parameter_handle) {
			tr069_handle->parameter_handle = tr069_parameter_uninit(tr069_handle->parameter_handle);
		}

		if (NULL != tr069_handle->tr111_handle) {
			tr069_handle->tr111_handle = tr111_control_uninit(tr069_handle->tr111_handle);
		}

		if (NULL != tr069_handle->control_handle) {
			tr069_handle->control_handle = tr069_control_uninit(tr069_handle->control_handle);
		}

//		ret = sem_getvalue(&tr069_handle->config_sem, &sem_value);
//		if (0 == ret) {
//			sem_timedwait(&tr069_handle->config_sem, &time_out);
//			sem_getvalue(&tr069_handle->config_sem, &cpe_dump->config_sem_value);
//			sem_destroy(&tr069_handle->config_sem);
//			cpe_dump->config_sem_value = -1;
//		}

		free(tr069_handle);
	}

	tr069_debug("ok quit\n");
	return NULL;
}

INT32_T tr069_load_environment(tr069_mgr *tr069_handle)
{
	INT32_T ret;
	INT32_T i;
	INT8_T string_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	INT8_T temp_param_name[TR069_MAX_PARAM_NAME_LEN] = {0};
	tr069_control_mgr *control_handle = NULL;

	f_assert(tr069_handle);
	if (NULL == tr069_handle) {
		tr069_error("tr069 handle error\n");
		return -1;
	}

	control_handle = tr069_handle->control_handle;
	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}
	/* load request queue */
	strncpy(string_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_REQ_QUEUE, ""), sizeof(string_buff) - 1);
	ret = 0;
	if (ret != 0) {
		tr069_error("load request queue error\n");
		goto ERROR_EXIT;
	}

	for (i = 0; i < strlen(string_buff); i++) {
		switch (string_buff[i]) {
		case '2':
			/* TR069_RPC_ACS_TRANSFER_COMPLETE */
			tr069_control_put_request(control_handle, TR069_RPC_ACS_TRANSFER_COMPLETE);
			break;

		default:
			break;
		}
	}

	/* load event */
	strncpy(string_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_EVENTFLAG, TR069_DEFAULT_EVENT_FLAG), sizeof(string_buff) - 1);
	ret = 0;
	if (ret != 0) {
		tr069_error("load event flag error\n");
		goto ERROR_EXIT;
	}

	for (i = TR069_EVENT_BOOTSTRAP; i < TR069_EVENT_NUMBER; i++) {
		if ('1' == string_buff[i]) {
			/* this event is triggered */
			tr069_control_set_event(control_handle, i, 1, NULL);
		} else {
			tr069_control_set_event(control_handle, i, 0, NULL);
		}
	}

	/* load commandkey */
	strncpy(string_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_CMDKEY, ""), sizeof(string_buff) - 1);
	ret = 0;
	if (ret != 0) {
		tr069_error("load command key error\n");
		goto ERROR_EXIT;
	}

	strncpy(control_handle->event.command_key, string_buff, sizeof(control_handle->event.command_key) - 1);

	/* load inform parameters */
	strncpy(string_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_INFORM_PARAMS, ""), sizeof(string_buff) - 1);
	ret = 0;
	if (ret != 0) {
		tr069_error("load inform parameters error\n");
		goto ERROR_EXIT;
	}

	for (i = 0; i < strlen(string_buff); i++) {
		sprintf(temp_param_name, "%c", string_buff[i]);
		tr069_control_put_inform_parameter(control_handle, temp_param_name);
	}

	/* load transfer start time */
	strncpy(string_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_TRANSTIME, ""), sizeof(string_buff) - 1);
	ret = 0;
	if (ret != 0) {
		tr069_error("load transfer start time error\n");
		goto ERROR_EXIT;
	}
	strncpy(control_handle->transfer_stime, string_buff, sizeof(control_handle->transfer_stime) - 1);

	/* load transfer complete time */
	strncpy(string_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_TRANS_COMPLETE_TIME, ""), sizeof(string_buff) - 1);
	ret = 0;
	if (ret != 0) {
		tr069_error("load transfer complete time error\n");
		goto ERROR_EXIT;
	}
	strncpy(control_handle->transfer_ctime, string_buff, sizeof(control_handle->transfer_ctime) - 1);

	/* load transfer result fault code */
	strncpy(string_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_TRANS_RET, "0"), sizeof(string_buff) - 1);
	ret = 0;
	if (ret != 0) {
		tr069_error("load transfer result fault code error\n");
		goto ERROR_EXIT;
	}
	control_handle->transfer_result = atoi(string_buff);

	/* should clear environment in config */
	ret = iniparser_set(tr069_status_ini, TR069_STATUS_REQ_QUEUE, "");
	if (ret != 0)
		tr069_error("set request queue error\n");

	ret = iniparser_set(tr069_status_ini, TR069_STATUS_EVENTFLAG, TR069_DEFAULT_EVENT_FLAG);
	if (ret != 0)
		tr069_error("set event flag error\n");

	ret = iniparser_set(tr069_status_ini, TR069_STATUS_CMDKEY, "");
	if (ret != 0)
		tr069_error("set command key error\n");

	ret = iniparser_set(tr069_status_ini, TR069_STATUS_INFORM_PARAMS, "");
	if (ret != 0)
		tr069_error("set inform parameters error\n");

	ret = iniparser_set(tr069_status_ini, TR069_STATUS_TRANSTIME, "");
	if (ret != 0)
		tr069_error("set transfer start time error\n");

	ret = iniparser_set(tr069_status_ini, TR069_STATUS_TRANS_COMPLETE_TIME, "");
	if (ret != 0)
		tr069_error("set transfer complete time error\n");

	ret = iniparser_set(tr069_status_ini, TR069_STATUS_TRANS_RET, "0");
	if (ret != 0)
		tr069_error("set transfer result error\n");

	tr069_save_status();

	tr069_debug("load environment success\n");
	return 0;

ERROR_EXIT:

	tr069_error("load environment error\n");
	return -1;
}

INT32_T tr069_dump_environment(tr069_mgr *tr069_handle)
{
	INT8_T string_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	INT32_T ret, i;
	tr069_control_mgr *control_handle = NULL;
	tr069_rpc_type request;

	tr069_debug("enter\n");

	f_assert(tr069_handle);
	if (NULL == tr069_handle) {
		tr069_error("tr069 handle error\n");
		return -1;
	}

	control_handle = tr069_handle->control_handle;
	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	/* dump event flag */
	/* first load current value from config */
	memset(string_buff, 0, TR069_EVENT_NUMBER);
	strncpy(string_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_EVENTFLAG, TR069_DEFAULT_EVENT_FLAG), sizeof(string_buff) - 1);
	ret = 0;
	/* then set and save to config */
	for (i = TR069_EVENT_BOOTSTRAP; i < TR069_EVENT_NUMBER; i++) {
		if (1 == control_handle->event.event_flag[i]) {
			string_buff[i] = '1';
		} else if (string_buff[i] != '1') {
			string_buff[i] = '0';
		}
	}
	string_buff[i] = 0;
	string_buff[TR069_EVENT_NUMBER] = 0;
	printf("------%s\n", string_buff);
	ret = iniparser_set(tr069_status_ini, TR069_STATUS_EVENTFLAG, string_buff);

	/* dump request queue */
	/* first load current value from config */
	string_buff[0] = 0;
	strncpy(string_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_REQ_QUEUE, ""), sizeof(string_buff) - 1);
	ret = 0;
	/* then set and save to config */
	i = strlen(string_buff);
	while ((i + 1) < TR069_MAX_REQUEST_NUM) {
		request = tr069_control_pick_request(control_handle);
		/* todo: add more supported request */
		if (TR069_RPC_ACS_TRANSFER_COMPLETE == request) {
			string_buff[i] = '2';
			i++;
		} else {
			/* request queue empty */
			break;
		}
	}
	string_buff[i] = 0;
	string_buff[TR069_MAX_REQUEST_NUM] = 0;

	ret = iniparser_set(tr069_status_ini, TR069_STATUS_REQ_QUEUE, string_buff);

	/* dump inform parameters */
	/* first load current value from config */
	string_buff[0] = 0;
	strncpy(string_buff, iniparser_getstring(tr069_status_ini, TR069_STATUS_INFORM_PARAMS, ""), sizeof(string_buff) - 1);
	ret = 0;
	/* then set and save to config */
	INT8_T *param_code;
	for (i = 0; i < control_handle->inform_parameters.number; i++) {
		param_code = control_handle->inform_parameters.codes[i];
		sprintf(string_buff + strlen(string_buff), "%s", param_code);
	}
	
	ret = iniparser_set(tr069_status_ini, TR069_STATUS_INFORM_PARAMS, string_buff);

	/* dump command key */
	ret = iniparser_set(tr069_status_ini, TR069_STATUS_CMDKEY, control_handle->event.command_key);

	/* dump transfer start time */
	ret = iniparser_set(tr069_status_ini, TR069_STATUS_TRANSTIME, control_handle->transfer_stime);

	/* dump transfer complete time */
	ret = iniparser_set(tr069_status_ini, TR069_STATUS_TRANS_COMPLETE_TIME, control_handle->transfer_ctime);

	/* dump transfer result fault code */
	sprintf(string_buff, "%d", control_handle->transfer_result);
	ret = iniparser_set(tr069_status_ini, TR069_STATUS_TRANS_RET, string_buff);

	tr069_debug("dump environment success\n");
	return 0;

	goto ERROR_EXIT;

ERROR_EXIT:
	tr069_error("dump environment error\n");
	return -1;
}
