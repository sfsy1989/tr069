/********************************************************************
    Created By Ash, 2009/11/25
    Model: Tr069
    Sub Model: Control
    Function: Main task control and execute
********************************************************************/
#include <n_lib.h>
#include <time.h>

#include "tr069.h"
#include "tr069_control.h"
#include "tr069_session.h"
#include "tr069_dump.h"
#include "tr069_service.h"
#include "tr069_ping.h"
#include "tr069_traceroute.h"
#include "tr069_cookie.h"
#include "tr069_timer.h"
#include "tr069_speed_test.h"

#define FV_LOG_FIRST_BOOT_FLAG_SIZE     8
#define FV_LOG_CMD_SIZE                 256
/* global */

/* extern */
extern int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
extern int tr069_set_config(int key, char *value);
extern int tr069_get_config(int key, char *value, unsigned int size);

extern tr069_mgr *g_tr069;
extern cpe_dump_s *cpe_dump;
extern int g_tr069_stand;
extern INT8_T* tr069_parameter_name_table[TR069_PARAM_NUM];
extern int g_speedtest_flag;
/* local */
int g_tr069_timer;

tr069_control_mgr* tr069_control_init(tr069_mgr *tr069_handle)
{
	tr069_control_mgr *manager = NULL;

	tr069_debug("enter\n");
	if (NULL == tr069_handle) {
		tr069_error("tr069 handle error\n");
		goto ERROR_EXIT;
	}

	/* initialize control manager */
	manager = malloc(sizeof(tr069_control_mgr));
	if (NULL == manager) {
		tr069_error("malloc control manager error\n");
		goto ERROR_EXIT;
	}
	memset(manager, 0, sizeof(tr069_control_mgr));

	/* initialize all elements */
	manager->tr069_handle = tr069_handle;
	manager->status = TR069_CONTROL_STATUS_STOP;
	manager->current_task = TR069_TASK_NONE;
	manager->finish_flag = 0;

//	ret = sem_init(&manager->event_sem, 0, 1);
//	if (ret != 0) {
//		f_error(N_LOG_CPE, "initialize event semaphore error\n");
//		goto ERROR_EXIT;
//	}

//	ret = sem_init(&manager->task_queue_sem, 0, 1);
//	if (ret != 0) {
//		f_error(N_LOG_CPE, "initialize task queue semaphore error\n");
//		goto ERROR_EXIT;
//	}

//	ret = sem_init(&manager->request_queue_sem, 0, 1);
//	if (ret != 0) {
//		f_error(N_LOG_CPE, "initialize request queue semaphore error\n");
//		goto ERROR_EXIT;
//	}

//	ret = sem_init(&manager->inform_parameters_sem, 0, 1);
//	if (ret != 0) {
//		f_error(N_LOG_CPE, "initialize inform parameters semaphore error\n");
//		goto ERROR_EXIT;
//	}

//	ret = sem_init(&manager->session_sem, 0, 1);
//	if (ret != 0) {
//		f_error(N_LOG_CPE, "initialize session semaphore error\n");
//		goto ERROR_EXIT;
//	}
//	sem_getvalue(&manager->session_sem, &cpe_dump->session_sem_value);

	tr069_control_clear_event(manager);
	tr069_control_clear_task(manager);
	tr069_control_clear_request(manager);
	tr069_control_clear_inform_parameters(manager);

	tr069_debug("ok quit\n");
	return manager;

ERROR_EXIT:

	/* if control manger already initialized, uninitialize it */
	if (manager != NULL) {
		manager = tr069_control_uninit(manager);
	}

	tr069_error("error quit\n");
	return NULL;
}

tr069_control_mgr* tr069_control_uninit(tr069_control_mgr *control_handle)
{
	struct timespec time_out;

	time_out.tv_sec = time(NULL) + 1;
	time_out.tv_nsec = 0;

	tr069_debug("enter\n");
	if (control_handle != NULL) {
		#if 0
		/* uninitialize all elements */
		ret = sem_getvalue(&control_handle->event_sem, &sem_value);
		if (0 == ret) {
			sem_timedwait(&control_handle->event_sem, &time_out);
			sem_destroy(&control_handle->event_sem);
		}

		ret = sem_getvalue(&control_handle->task_queue_sem, &sem_value);
		if (0 == ret) {
			sem_timedwait(&control_handle->task_queue_sem, &time_out);
			sem_destroy(&control_handle->task_queue_sem);
		}

		ret = sem_getvalue(&control_handle->request_queue_sem, &sem_value);
		if (0 == ret) {
			sem_timedwait(&control_handle->request_queue_sem, &time_out);
			sem_destroy(&control_handle->request_queue_sem);
		}

		ret = sem_getvalue(&control_handle->inform_parameters_sem, &sem_value);
		if (0 == ret) {
			sem_timedwait(&control_handle->inform_parameters_sem, &time_out);
			sem_destroy(&control_handle->inform_parameters_sem);
		}

		ret = sem_getvalue(&control_handle->session_sem, &sem_value);
		if (0 == ret) {
			sem_timedwait(&control_handle->session_sem, &time_out);
			sem_getvalue(&control_handle->session_sem, &cpe_dump->session_sem_value);
			sem_destroy(&control_handle->session_sem);
			cpe_dump->session_sem_value = -1;
		}
		#endif

		free(control_handle);
	}

	tr069_debug("ok quit\n");
	return NULL;
}

INT32_T tr069_control_set_event(tr069_control_mgr *control_handle, INT32_T event_index, INT8_T event_value, INT8_T *command_key)
{
	tr069_debug("enter\n");

	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	if (event_index < 0 || event_index >= TR069_EVENT_NUMBER) {
		tr069_error("event index(%d) error\n", event_index);
		return -1;
	}

	if (0 != event_value && 1 != event_value) {
		tr069_error("event value(%d) error\n", event_value);
		return -1;
	}


	/* set event flag */
	if (0 == control_handle->event.event_flag[event_index] && 1 == event_value) {
		control_handle->event.number += 1;
	}
	if (1 == control_handle->event.event_flag[event_index] && 0 == event_value) {
		control_handle->event.number -= 1;
	}
	control_handle->event.event_flag[event_index] = event_value;

	/* set command key */
	if (command_key != NULL) {
		strncpy(control_handle->event.command_key, command_key, sizeof(control_handle->event.command_key) - 1);
	}

	return 0;
}

INT32_T tr069_control_clear_event(tr069_control_mgr *control_handle)
{
	tr069_debug("enter\n");
	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}
	/* clear */
	memset(control_handle->event.event_flag, 0, sizeof(control_handle->event.event_flag));
	control_handle->event.number = 0;

	tr069_debug("ok quit\n");
	return 0;

}

INT32_T tr069_control_get_event(tr069_control_mgr *control_handle, tr069_event *event_recver)
{
	tr069_debug("enter\n");
	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	if (NULL == event_recver) {
		tr069_error("event receive buffer error\n");
		return -1;
	}


	/* copy to receive pointer */
	memcpy(event_recver, &control_handle->event, sizeof(tr069_event));


	tr069_debug("ok quit\n");
	return 0;
}

INT32_T tr069_control_put_task(tr069_control_mgr *control_handle, tr069_task_type task)
{
	//INT32_T i;
	tr069_task_queue *task_queue = NULL;

	tr069_debug("enter(%d)\n", task);
	/* check for validity */
	if (task <= TR069_TASK_NONE || task >= TR069_TASK_NUMBER) {
		tr069_error("invalid task(%d)\n", task);
		return -1;
	}

	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	task_queue = &control_handle->task_queue;


	/* check for full */
	if ((task_queue->tail + 1) % TR069_MAX_TASK_NUM == task_queue->head) {
		tr069_error("task queue full\n");
		goto ERROR_EXIT;
	}

	/* if there already have session task, don't put */
	/* don't check multiple task
	i = task_queue->head;
	while (i != task_queue->tail)
	{
	    if (TR069_TASK_SESSION == task_queue->queue[i])
	    {
	        f_logger(LOG_ERR, "[tr069_control_put_task] multiple session task\n");
	        goto ERROR_EXIT;
	    }

	    i = (i + 1) % TR069_MAX_TASK_NUM;
	}
	*/

	/* put task into tail */
	task_queue->queue[(int)task_queue->tail] = task;
	task_queue->tail = (task_queue->tail + 1) % TR069_MAX_TASK_NUM;


	tr069_debug("ok quit\n");
	return 0;

ERROR_EXIT:


	tr069_error("error quit\n");
	return -1;
}

INT32_T tr069_control_put_request(tr069_control_mgr *control_handle, tr069_rpc_type request)
{
	tr069_request_queue *request_queue = NULL;

	tr069_debug("enter(%d)\n", request);
	/* check for validity */
	/* not allow empty and inform */
	if (request <= TR069_RPC_ACS_INFORM || request > TR069_RPC_ACS_TRANSFER_COMPLETE) {
		tr069_error("invalid request(%d)\n", request);
		return -1;
	}

	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	request_queue = &control_handle->request_queue;


	/* check for full */
	if ((request_queue->tail + 1) % TR069_MAX_REQUEST_NUM == request_queue->head) {
		tr069_error("request queue full\n");
		goto ERROR_EXIT;
	}

	/* put request into tail */
	request_queue->queue[(int)request_queue->tail] = request;
	request_queue->tail = (request_queue->tail + 1) % TR069_MAX_REQUEST_NUM;


	tr069_debug("ok quit\n");
	return 0;

ERROR_EXIT:


	tr069_error("error quit\n");
	return -1;
}

tr069_task_type tr069_control_pick_task(tr069_control_mgr *control_handle)
{
	tr069_task_queue *task_queue = NULL;
	tr069_task_type task;

	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	task_queue = &control_handle->task_queue;


	/* check for empty */
	if (task_queue->tail == task_queue->head) {
		goto ERROR_EXIT;
	}

	/* get task from head */
	task = task_queue->queue[(int)task_queue->head];
	task_queue->head = (task_queue->head + 1) % TR069_MAX_TASK_NUM;

	return task;

ERROR_EXIT:

	return TR069_TASK_NONE;
}

tr069_rpc_type tr069_control_pick_request(tr069_control_mgr *control_handle)
{
	tr069_request_queue *request_queue = NULL;
	tr069_rpc_type request;

	tr069_debug("enter\n");
	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	request_queue = &control_handle->request_queue;

	/* check for empty */
	if (request_queue->tail == request_queue->head) {
		tr069_error("request queue empty\n");
		goto ERROR_EXIT;
	}

	/* get request from head */
	request = request_queue->queue[(int)request_queue->head];
	request_queue->head = (request_queue->head + 1) % TR069_MAX_REQUEST_NUM;

	tr069_debug("ok quit\n");
	return request;

ERROR_EXIT:

	tr069_error("error quit\n");
	return -1;
}

INT32_T tr069_control_clear_task(tr069_control_mgr *control_handle)
{
	tr069_task_queue *task_queue = NULL;

	tr069_debug("enter\n");
	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	task_queue = &control_handle->task_queue;

	/* clear task queue */
	memset(task_queue, 0, sizeof(tr069_task_queue));
	task_queue->head = 0;
	task_queue->tail = 0;

	tr069_debug("ok quit\n");
	return 0;

}

INT32_T tr069_control_clear_request(tr069_control_mgr *control_handle)
{
	tr069_request_queue *request_queue = NULL;

	tr069_debug("enter\n");
	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	request_queue = &control_handle->request_queue;

	/* clear request queue */
	memset(request_queue, 0, sizeof(tr069_request_queue));
	request_queue->head = 0;
	request_queue->tail = 0;

	tr069_debug("ok quit\n");
	return 0;
}

INT32_T tr069_control_put_inform_parameter(tr069_control_mgr *control_handle, INT8_T *param_code)
{
	tr069_inform_parameters *inform_parameters = NULL;
	INT8_T *param_name = NULL;
	INT32_T code;

	if (NULL == param_code) {
		tr069_error("parameter code null\n");
		return -1;
	}

	code = param_code[0] - '!';
	if (code < 0 || code >= (sizeof(tr069_parameter_name_table) / sizeof(INT8_T*))) {
		tr069_error("parameter code(%d) error\n", code);
		return -1;
	}

	param_name = tr069_parameter_name_table[code];
	tr069_debug("param_name:(%s)",param_name);

	tr069_debug("enter(%s)\n", param_code);
	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	if (NULL == param_name || NULL == param_code) {
		tr069_error("parameter name error\n");
		return -1;
	}

	inform_parameters = &control_handle->inform_parameters;


	/* check for full */
	if (inform_parameters->number >= TR069_MAX_INFORM_PARAM_NUM) {
		tr069_error("inform parameters full");
		goto ERROR_EXIT;
	}

	/* insert parameter to end of list */
	strncpy(inform_parameters->parameters[(int)inform_parameters->number], param_name, TR069_MAX_PARAM_NAME_LEN - 1);
	strncpy(inform_parameters->codes[(int)inform_parameters->number], param_code, TR069_MAX_PARAM_CODE_LEN - 1);
	inform_parameters->number++;


	tr069_debug("ok quit\n");
	return 0;

ERROR_EXIT:


	tr069_error("error quit\n");
	return -1;
}

INT32_T tr069_control_get_inform_parameters(tr069_control_mgr *control_handle, tr069_inform_parameters *parameters_recver)
{
	tr069_debug("enter\n");
	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	if (NULL == parameters_recver) {
		tr069_error("parameter receive buffer error\n");
		return -1;
	}


	/* copy to receive pointer */
	memcpy(parameters_recver, &control_handle->inform_parameters, sizeof(tr069_inform_parameters));

	/* it maybe necessary to clear */


	tr069_debug("ok quit\n");
	return 0;
}

INT32_T tr069_control_clear_inform_parameters(tr069_control_mgr *control_handle)
{
	tr069_inform_parameters *inform_parameters = NULL;

	tr069_debug("enter\n");
	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	inform_parameters = &control_handle->inform_parameters;

	/* clear inform parameters */
	memset(inform_parameters, 0, sizeof(tr069_inform_parameters));
	inform_parameters->number = 0;

	tr069_debug("ok quit\n");
	return 0;
}

INT32_T tr069_control_get_event_number(tr069_event *event)
{
	INT32_T i, num;

	if (NULL == event) {
		tr069_error("event pointer error\n");
		return -1;
	}

	num = 0;
	for (i = TR069_EVENT_BOOTSTRAP; i < TR069_EVENT_NUMBER; i++) {
		if (1 == event->event_flag[i]) {
			/* an event has been triggered */
			num++;
		}
	}

	return num;
}

INT32_T tr069_control_is_event_empty(tr069_event *event)
{
	INT32_T i;

	if (NULL == event) {
		tr069_error("event pointer error\n");
		return -1;
	}

	for (i = TR069_EVENT_BOOTSTRAP; i < TR069_EVENT_M_SCHEDULEINFORM; i++) {
		if (1 == event->event_flag[i]) {
			/* a primary event has been triggered */
			return 0;
		}
	}

	return 1;
}

INT32_T tr069_control_ping()
{
	INT8_T string_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	INT32_T ret = -1;
	tr069_ping_argument argument;
	tr069_ping_result result;
	tr069_get_config(TR069_PARAM_PingDiagState, string_buff, sizeof(string_buff));
	if (strcmp("Requested", string_buff) != 0) {
		tr069_error("state error\n");
		return -1;
	}

	tr069_get_config(TR069_PARAM_PingDiagDataBlockSize, string_buff, sizeof(string_buff));
	argument.data_size = atoi(string_buff);

	tr069_get_config(TR069_PARAM_PingDiagReptNumber, string_buff, sizeof(string_buff));
	argument.ping_count = atoi(string_buff);

	tr069_get_config(TR069_PARAM_PingDiagTimeout, string_buff, sizeof(string_buff));
	argument.time_out = atoi(string_buff);

	tr069_get_config(TR069_PARAM_PingDiagHost, string_buff, sizeof(string_buff));

	ret = tr069_ping(string_buff, &argument, &result);

	if (0 == ret) {
		tr069_set_config(TR069_PARAM_PingDiagState, "Complete");

		sprintf(string_buff, "%d", result.success_count);
		tr069_set_config(TR069_PARAM_PingDiagSuccessCount, string_buff);

		sprintf(string_buff, "%d", result.failure_count);
		tr069_set_config(TR069_PARAM_PingDiagFailureCount, string_buff);

		sprintf(string_buff, "%f", result.min_time);
		tr069_set_config(TR069_PARAM_PingDiagMinRespTime, string_buff);

		sprintf(string_buff, "%f", result.avg_time);
		tr069_set_config(TR069_PARAM_PingDiagAvgRespTime, string_buff);

		sprintf(string_buff, "%f", result.max_time);
		tr069_set_config(TR069_PARAM_PingDiagMaxRespTime, string_buff);

	} else {
		tr069_set_config(TR069_PARAM_PingDiagState, "Error_Internal");

		sprintf(string_buff, "%d", 0);
		tr069_set_config(TR069_PARAM_PingDiagSuccessCount, string_buff);

		sprintf(string_buff, "%d", argument.ping_count);
		tr069_set_config(TR069_PARAM_PingDiagFailureCount, string_buff);

		sprintf(string_buff, "%f",(float) 0);
		tr069_set_config(TR069_PARAM_PingDiagMinRespTime, string_buff);

		sprintf(string_buff, "%f",(float) 0);
		tr069_set_config(TR069_PARAM_PingDiagAvgRespTime, string_buff);

		sprintf(string_buff, "%f",(float) 0);
		tr069_set_config(TR069_PARAM_PingDiagMaxRespTime, string_buff);

		tr069_error("ping error\n");
	}
	return 0;
}

extern void tr069_clear_route_host();

INT32_T tr069_control_tracert()
{
	INT8_T string_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	INT32_T ret = -1;
	f_tracert_argument argument;
	f_tracert_result result;

	tr069_get_config(TR069_PARAM_TracertDiagState, string_buff, sizeof(string_buff));
	if (strcmp("Requested", string_buff) != 0) {
		tr069_error("state error\n");
		return -1;
	}

	tr069_get_config(TR069_PARAM_TracertDiagDataBlockSize, string_buff, sizeof(string_buff));
	argument.data_size = atoi(string_buff);

	tr069_get_config(TR069_PARAM_TracertDiagMaxHopCount, string_buff, sizeof(string_buff));
	argument.max_hop_count = atoi(string_buff);

	tr069_get_config(TR069_PARAM_TracertDiagTimeout, string_buff, sizeof(string_buff));
	argument.time_out = atoi(string_buff);

	tr069_get_config(TR069_PARAM_TracertDiagHost, string_buff, sizeof(string_buff));

	ret = tr069_tracert(string_buff, &argument, &result);

	if (0 == ret) {
		tr069_set_config(TR069_PARAM_TracertDiagState, "Complete");

		sprintf(string_buff, "%d", result.hop_number);
		tr069_set_config(TR069_PARAM_TracertDiagHopsNumber, string_buff);

		sprintf(string_buff, "%d", result.response_time);
		tr069_set_config(TR069_PARAM_TracertDiagRespTime, string_buff);

		tr069_clear_route_host();
		int i;
		for (i = 0; i < result.hop_number; i++) {
			tr069_set_config(TR069_PARAM_HOPHOST1 + i, result.hophost[i]);
		}
	} else {
		tr069_set_config(TR069_PARAM_TracertDiagState, "Error_Internal");

		sprintf(string_buff, "%d", result.hop_number);
		tr069_set_config(TR069_PARAM_TracertDiagHopsNumber, string_buff);

		sprintf(string_buff, "%d", result.response_time);
		tr069_set_config(TR069_PARAM_TracertDiagRespTime, string_buff);

		tr069_clear_route_host();

		tr069_error("tracert error\n");
	}
	return FV_OK;
}

static int tr069_control_picktask_proc(void *arg)
{
	static INT32_T ret;
	static tr069_control_mgr *control_handle;
	static tr069_inform_parameters current_inform_parameters;
	static tr069_event current_event;
	static tr069_mgr *tr069_handle = NULL;
	static INT8_T target_url[F_MAX_URL_SIZE] = {0};
	static INT8_T* ftp_s = "ftp://";

	control_handle = (tr069_control_mgr *)arg;
	/* check for task queue */
	control_handle->current_task = tr069_control_pick_task(control_handle);
	tr069_handle = control_handle->tr069_handle;
	if (NULL == tr069_handle) {
		tr069_error("tr069 handle error\n");
		goto FINISH_EXIT;
	}

	if (NULL == tr069_handle->session_handle) {
		tr069_error("session handle error\n");
		goto FINISH_EXIT;
	}
	/* switch task to execute */
	switch (control_handle->current_task) {
	case TR069_TASK_SESSION:
		/* session task, check for event */
		/* first check for calling */
		if (g_tr069->call_flag <= 0) {
			tr069_error("task without setting(call = %d)\n", g_tr069->call_flag);
			break;
		}
		g_tr069->call_flag--;
		cpe_dump->call_flag = g_tr069->call_flag;
		if (g_tr069->call_flag > 0) {
			tr069_error("other proccess still setting(call = %d)\n", g_tr069->call_flag);
			break;
		}

		ret = tr069_control_get_event(control_handle, &current_event);
		if (ret != 0) {
			/* break main task if get event error */
			tr069_error("get event error\n");
			break;
		}
		tr069_control_clear_event(control_handle);

		if (tr069_control_is_event_empty(&current_event) != 0) {
			/* break main task if no event */
			tr069_error("no event\n");
			break;
		}

		ret = tr069_control_get_inform_parameters(control_handle, &current_inform_parameters);
		if (ret != 0) {
			/* break main task if get inform parameters error */
			tr069_error("get inform parameters error\n");
			break;
		}
		tr069_control_clear_inform_parameters(control_handle);

		ret = tr069_session_clear(tr069_handle->session_handle);
		control_handle->status = TR069_CONTROL_STATUS_TASK;

		/* session sem off
		sem_trywait(&control_handle->session_sem);
		sem_getvalue(&control_handle->session_sem, &cpe_dump->session_sem_value);
		sem_getvalue(&control_handle->session_sem, &cpe_dump->session_sem_value);
		*/

		/* execute session here */
		ret = tr069_session_main(tr069_handle->session_handle, &current_event, &current_inform_parameters);

		control_handle->status = TR069_CONTROL_STATUS_IDLE;
		break;

	case TR069_TASK_REBOOT:
		tr069_save_cookie(tr069_handle->rpc_handle);
		tr069_control_set_event(control_handle, TR069_EVENT_M_REBOOT, 1, NULL);

		control_handle->status = TR069_CONTROL_STATUS_TASK;

		n_osd_restart();
		n_osd_show_prompt(OSD_OP_SHOW, N_OSD_PT_REBOOT);
		sleep(2);
		//n_device_reboot();
		system("kill -3 `pidof com.fonsview.tr069`");
		system("kill -2 `pidof com.fonsview.config`");
		system("am broadcast -a android.intent.action.REBOOT --ei nowait 1 --ei interval 1 --ei window 0");

		control_handle->status = TR069_CONTROL_STATUS_IDLE;
		break;

	case TR069_TASK_FACTORY_RESET:
		tr069_debug("execute factory reset\n");
		tr069_save_cookie(tr069_handle->rpc_handle);

		control_handle->status = TR069_CONTROL_STATUS_TASK;

		/* execute factory reset here */
		//n_cfg_factory_reset();
		tr069_service_set_status(TR069_STATUS_FIRSTBOOT, "1");

		n_osd_restart();
		n_osd_show_prompt(OSD_OP_SHOW, N_OSD_PT_RESET);
		sleep(2);
		//n_device_reboot();
		system("am broadcast -a android.intent.action.MASTER_CLEAR");

		control_handle->status = TR069_CONTROL_STATUS_IDLE;
		break;

	case TR069_TASK_DOWNLOAD:
		tr069_debug("execute download\n");
		tr069_save_cookie(tr069_handle->rpc_handle);

		control_handle->status = TR069_CONTROL_STATUS_TASK;

		/* execute download here */
		if (0 == strncasecmp(control_handle->download_argument.url, ftp_s, strlen(ftp_s)) &&
			(0 != control_handle->download_argument.username[0] ||
			0 != control_handle->download_argument.password[0])) {
			snprintf(target_url, sizeof(target_url), "ftp://%s:%s@%s",
				control_handle->download_argument.username,
				control_handle->download_argument.password,
				control_handle->download_argument.url + strlen(ftp_s));
		}
		else {
			snprintf(target_url, sizeof(target_url), "%s", control_handle->download_argument.url);
		}

		tr069_debug("download url = %s\n", target_url);
		//n_brows_tr069_upgrade(target_url, control_handle->download_argument.target_file_name);
		char cmd[N_MAX_SHELL_CMD_SIZE];
		snprintf(cmd, sizeof(cmd), "am broadcast -a com.fonsview.stb.OTA_ACTION --es PackageUrl %s --ei ForceType 0",target_url);
		tr069_debug("download cmd = %s\n", cmd);
		system(cmd);

		control_handle->status = TR069_CONTROL_STATUS_IDLE;
		break;

	case TR069_TASK_UPLOAD:
		tr069_debug("execute upload\n");
		/* upload task */
		control_handle->status = TR069_CONTROL_STATUS_TASK;

		/* execute upload here */
		if (0 == strncasecmp(control_handle->upload_argument.url, ftp_s, strlen(ftp_s)) &&
			(0 != control_handle->upload_argument.username[0] ||
			0 != control_handle->upload_argument.password[0])) {
			snprintf(target_url, sizeof(target_url), "ftp://%s:%s@%s",
				control_handle->upload_argument.username,
				control_handle->upload_argument.password,
				control_handle->upload_argument.url + strlen(ftp_s));
		}
		else {
			snprintf(target_url, sizeof(target_url), "%s", control_handle->upload_argument.url);
		}

		tr069_debug("upload url = %s\n", target_url);
		/* fixeme: upload delay not implemented */
		n_iptv_upload_statis_now(target_url);

		control_handle->status = TR069_CONTROL_STATUS_IDLE;
		break;

	case TR069_TASK_PING:
		tr069_debug("execute ping\n");
		/* ping task */
		control_handle->status = TR069_CONTROL_STATUS_TASK;

		/* execute ping here */
		ret = tr069_control_ping();
		if (0 == ret) {
			/* should trigger diagnostics complete event */
			tr069_service_set_event(TR069_EVENT_DIAGNOSTICS_COMPLETE, 1, "");
			tr069_control_put_task(control_handle, TR069_TASK_SESSION);
		}

		control_handle->status = TR069_CONTROL_STATUS_IDLE;
		break;

	case TR069_TASK_TRACERT:
		tr069_debug("execute trace route\n");
		/* trace route task */
		control_handle->status = TR069_CONTROL_STATUS_TASK;

		/* execute trace route here */
		ret = tr069_control_tracert();
		if (0 == ret) {
			/* should trigger diagnostics complete event */
			tr069_service_set_event(TR069_EVENT_DIAGNOSTICS_COMPLETE, 1, "");
			tr069_control_put_task(control_handle, TR069_TASK_SESSION);
		}

		control_handle->status = TR069_CONTROL_STATUS_IDLE;
		break;

	case TR069_TASK_SPEED_TEST:
		tr069_debug("execute speed test\n");
		/* speed test task */
		control_handle->status = TR069_CONTROL_STATUS_TASK;
		/* execute speed test here */
		ret = tr069_control_speed_test();
		
		control_handle->status = TR069_CONTROL_STATUS_IDLE;
		break;

	default:
		/* invalid task or no task */
		if (1 == control_handle->finish_flag) {
			tr069_debug("dbus called stop\n");
			goto FINISH_EXIT;
		}
		break;
	}

	return FV_OK;

FINISH_EXIT:
	/* dump environment into config */
	control_handle->status = TR069_CONTROL_STATUS_DUMP;
	control_handle->current_task = TR069_TASK_NONE;

	tr069_dump_environment(tr069_handle);

	control_handle->status = TR069_CONTROL_STATUS_QUIT;

	tr069_debug("ok quit\n");

	return -1;
}

INT32_T tr069_control_main_task(tr069_control_mgr *control_handle)
{
	tr069_event current_event;

	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	memset(&current_event, 0, sizeof(tr069_event));
	control_handle->status = TR069_CONTROL_STATUS_IDLE;

	tr069_control_check_first_boot();//check first boot

	tr069_start_heartbeat();

	/* should start with session task */
	//tr069_control_put_task(control_handle, TR069_TASK_SESSION);

	tr069_load_cookie(control_handle->tr069_handle->rpc_handle);
	tr069_control_get_event(control_handle, &current_event);
	if (0 == current_event.event_flag[TR069_EVENT_M_REBOOT]) {
		tr069_clear_cookie(control_handle->tr069_handle->rpc_handle);
	}

	g_tr069_timer = f_timer_add(500, tr069_control_picktask_proc, (void *)control_handle);
	if (g_tr069_timer <= 0) {
		tr069_error("f_timer_add failed.\n");
		return -1;
	}

	return FV_OK;
}

INT32_T tr069_control_check_first_boot()
{
    char *first_boot_flag = NULL;
	char connection_request_url[F_IPTV_URL_SIZE] = { 0 };
	char setauth_cmd[FV_LOG_CMD_SIZE] = { 0 };
	char cr_username[F_MAX_USERNAME_SIZE] = { 0 };
	char cr_password[F_MAX_PASSWORD_SIZE] = { 0 };
	char ipaddr[F_MAX_IP_ADDR_SIZE] = { 0 };
	char cr_productclass[F_MAX_VERSION_NUM_SIZE] = { 0 };
	int ret;

	// get tr069 auth info
	n_cfg_get_config(F_CFG_ConnectionRequestUsername, cr_username,
			 sizeof(cr_username));
	n_cfg_get_config(F_CFG_ConnectionRequestPassword, cr_password,
			 sizeof(cr_password));
	n_cfg_get_config(F_CFG_ProductClass, cr_productclass,
		     sizeof(cr_productclass));
	sprintf(setauth_cmd,
		"fv_httpd -A /data/iptv/tr069/.htpasswd %s %s %s",
		cr_productclass, cr_username, cr_password);
	system(setauth_cmd);

	n_cfg_get_config(F_CFG_STBID, cr_username,
			 sizeof(cr_username));
	// set connection request url
	int max_wait_time_us = 2000000;
	int wait_time_us = 0;
	while (wait_time_us < max_wait_time_us) {
		ret = n_cfg_get_config(F_CFG_IPAddressReal, ipaddr, sizeof(ipaddr));
		if (0 == ret)
			break;

		usleep(200000);
		wait_time_us += 200000;
	}
	if (ret != 0) {
		tr069_error("get sockaddr_in error\n");
		return -1;
	}
	sprintf(connection_request_url, "http://%s:8080/tr069.cgi", ipaddr);
	tr069_debug("connection request url set to %s\n", connection_request_url);
	n_cfg_set_config(F_CFG_ConnectionRequestURL, connection_request_url);

	tr069_debug("get tr069:firstboot...");
	ret = tr069_service_get_status(TR069_STATUS_FIRSTBOOT, &first_boot_flag);

	if ('1' == *first_boot_flag) {
		// first boot
		tr069_debug("set event:bootstrap...");
		ret = tr069_service_set_event(TR069_EVENT_BOOTSTRAP, 1, "");
		tr069_service_set_status(TR069_STATUS_FIRSTBOOT, "0");
	} else {
		// not first boot
		tr069_debug("set event:boot...");
		ret = tr069_service_set_event(TR069_EVENT_BOOT, 1, "");
	}

    free(first_boot_flag);

	if (0 == ret) {
		if (TR069_STAND_CU == g_tr069_stand) {
			ret = tr069_service_put_inform_parameter(PCODE_HardwareVersion);
			ret = tr069_service_put_inform_parameter(PCODE_SoftwareVersion);
			ret = tr069_service_put_inform_parameter(PCODE_ManageSerURL);
			ret = tr069_service_put_inform_parameter(PCODE_UDPConnectionRequestAddress);
			ret = tr069_service_put_inform_parameter(PCODE_NTPServer1);
			ret = tr069_service_put_inform_parameter(PCODE_DNSServer0Real);
			ret = tr069_service_put_inform_parameter(PCODE_DNSServer1Real);
			ret = tr069_service_put_inform_parameter(PCODE_STBID);
			ret = tr069_service_put_inform_parameter(PCODE_LogServerUrl);
			ret = tr069_service_put_inform_parameter(PCODE_OperatorInfo);
			ret = tr069_service_put_inform_parameter(PCODE_UpgradeURL);
			ret = tr069_service_put_inform_parameter(PCODE_AuthURL);
			ret = tr069_service_put_inform_parameter(PCODE_BrowserURL2);
			ret = tr069_service_put_inform_parameter(PCODE_UserProvince);
			ret = tr069_service_put_inform_parameter(PCODE_ConnectionRequestURL);
			ret = tr069_service_put_inform_parameter(PCODE_UserID);
		} else {
			ret = tr069_service_put_inform_parameter(PCODE_HardwareVersion);
			ret = tr069_service_put_inform_parameter(PCODE_SoftwareVersion);
			ret = tr069_service_put_inform_parameter(PCODE_ConnectionRequestURL);
			ret = tr069_service_put_inform_parameter(PCODE_STBID);
			ret = tr069_service_put_inform_parameter(PCODE_UserID);
			ret = tr069_service_put_inform_parameter(PCODE_ProvisioningCode);
			ret = tr069_service_put_inform_parameter(PCODE_IPAddressReal);
			ret = tr069_service_put_inform_parameter(PCODE_MACAddress);
		}

		ret = tr069_service_start();
	}

	tr069_debug("check first boot return\n");
	return 0;
}

INT32_T tr069_control_speed_test()
{
	int ret;
	INT8_T string_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	struct speed_test_infos info;

	tr069_get_config(TR069_PARAM_SpeedTestEnable, string_buff, sizeof(string_buff));
	if (!strncasecmp(string_buff, "true", sizeof("true"))) {
		tr069_get_config(TR069_PARAM_SpeedTestDuration, string_buff, sizeof(string_buff));
		ret = atoi(string_buff);
		if (ret <= 0)
			return -1;
		else if (ret > MAX_DURATION)
			info.duration = MAX_DURATION;
		else
			info.duration = ret;
		
		tr069_get_config(TR069_PARAM_SpeedTestPath, info.filepath, sizeof(info.filepath));
		
		//strncpy(info.filepath, string_buff, sizeof(info.filepath));
		tr069_speed_test(&info);
		system("/ipstb/bin/go http://127.0.0.1/stb_config/speed_test.html");
		return 0;
		}
	return -1;
}

