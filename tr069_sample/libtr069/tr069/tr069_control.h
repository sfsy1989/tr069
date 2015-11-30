/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: Control
	Function: Main task control and execute
********************************************************************/
#ifndef _TR069_CONTROL_
#define _TR069_CONTROL_

#include <n_stb_tr069.h>
#include "tr069_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* max task number */
#define TR069_MAX_TASK_NUM 16
/* max request nubmer */
#define TR069_MAX_REQUEST_NUM 16
/* time(usec) idle before quit control main task */
#define TR069_MAX_IDLE_TIME_US 5000000

/* work status types of control manager */
typedef enum
{
	/* not under task cycle */
	TR069_CONTROL_STATUS_STOP = 0,
	/* waiting for task to execute */
	TR069_CONTROL_STATUS_IDLE,
	/* dealing with task execution */
	TR069_CONTROL_STATUS_TASK,
	/* dumping environment into config */
	TR069_CONTROL_STATUS_DUMP,
	/* have quited */
	TR069_CONTROL_STATUS_QUIT
}tr069_control_status;

/* task types in task queue */
typedef enum
{
	/* no task */
	TR069_TASK_NONE = -1,
	/* should check event and establish session with acs */
	TR069_TASK_SESSION,
	/* should reboot */
	TR069_TASK_REBOOT,
	/* should factory reset */
	TR069_TASK_FACTORY_RESET,
	/* should start download */
	TR069_TASK_DOWNLOAD,
	/* should start upload */
	TR069_TASK_UPLOAD,
	/* should start ping */
	TR069_TASK_PING,
	/* should start trace route */
	TR069_TASK_TRACERT,
	/* should start speed test */
	TR069_TASK_SPEED_TEST,
	
	TR069_TASK_NUMBER
}tr069_task_type;

/* flag for events triggered which should be informed */
typedef struct _tr069_event
{
	/* event flag, 0 for not triggered, 1 for triggered */
	INT8_T event_flag[TR069_EVENT_NUMBER];
	/* command key, can be empty string */
	INT8_T command_key[32];
	/* current event number */
	UINT8_T number;
}tr069_event;

/* contorl manager picks tasks from task queue and execute them */
typedef struct _tr069_task_queue
{
	/* task queue */
	tr069_task_type queue[TR069_MAX_TASK_NUM];
	/* head where task picked from */
	INT8_T head;
	/* tail where task put in */
	INT8_T tail;
}tr069_task_queue;

/* session manager picks requests from request queue and send rpc to acs */
typedef struct _tr069_request_queue
{
	/* request queue */
	tr069_rpc_type queue[TR069_MAX_REQUEST_NUM];
	/* head where request picked from */
	INT8_T head;
	/* tail where request put in */
	INT8_T tail;
}tr069_request_queue;

/* parameters which should be informed */
typedef struct _tr069_inform_parameters
{
	INT8_T parameters[TR069_MAX_INFORM_PARAM_NUM][TR069_MAX_PARAM_NAME_LEN];
	INT8_T codes[TR069_MAX_INFORM_PARAM_NUM][TR069_MAX_PARAM_CODE_LEN];
	INT8_T number;
}tr069_inform_parameters;

/* download argument from acs */
typedef struct _tr069_download_argument
{
	/* download file type */
	INT8_T file_type[64];
	/* download url */
	INT8_T url[256];
	/* download username */
	INT8_T username[256];
	/* download password */
	INT8_T password[256];
	/* download file size in bytes */
	INT32_T file_size;
	/* name of file to be used */
	INT8_T target_file_name[256];
	/* number of seconds before initiating download */
	INT32_T delay_seconds;
	/* redirect user's browser to this url while download succeed */
	INT8_T success_url[256];
	/* redirect user's browser to this url while download fail */
	INT8_T failure_url[256];
}tr069_download_argument;

/* upload argument from acs */
typedef struct _tr069_upload_argument
{
	/* upload file type */
	INT8_T file_type[64];
	/* upload url */
	INT8_T url[256];
	/* upload username */
	INT8_T username[256];
	/* upload password */
	INT8_T password[256];
	/* number of seconds before initiating upload */
	INT32_T delay_seconds;
}tr069_upload_argument;

/* control manager representing control sub model */
typedef struct _tr069_control_mgr
{
	/* handle to tr069 manager */
	tr069_mgr *tr069_handle;

	/* task queue */
	tr069_task_queue task_queue;
	/* request queue */
	tr069_request_queue request_queue;
	/* event */
	tr069_event event;
	/* inform parameters */
	tr069_inform_parameters inform_parameters;
	/* status */
	tr069_control_status status;
	/* current executing task */
	tr069_task_type current_task;
	/* transfer start time */
	INT8_T transfer_stime[32];
	/* transfer complete time */
	INT8_T transfer_ctime[32];
	/* transfer result fault code */
	tr069_fault_type transfer_result;
	/* download argument */
	tr069_download_argument download_argument;
	/* upload argument */
	tr069_upload_argument upload_argument;

	/* semaphore for event */
	sem_t event_sem;
	/* semaphore for task queue */
	sem_t task_queue_sem;
	/* semaphore for request queue */
	sem_t request_queue_sem;
	/* semaphore for inform parameters */
	sem_t inform_parameters_sem;
	/* semaphore for session execution */
	sem_t session_sem;
	/* finish flag */
	char finish_flag;
}tr069_control_mgr;

tr069_control_mgr* tr069_control_init(tr069_mgr *tr069_handle);

tr069_control_mgr* tr069_control_uninit(tr069_control_mgr *control_handle);

INT32_T tr069_control_set_event(tr069_control_mgr *control_handle, INT32_T event_index, INT8_T event_value, INT8_T *command_key);

INT32_T tr069_control_clear_event(tr069_control_mgr *control_handle);

INT32_T tr069_control_get_event(tr069_control_mgr *control_handle, tr069_event *event_recver);

INT32_T tr069_control_put_task(tr069_control_mgr *control_handle, tr069_task_type task);

INT32_T tr069_control_put_request(tr069_control_mgr *control_handle, tr069_rpc_type request);

tr069_task_type tr069_control_pick_task(tr069_control_mgr *control_handle);

tr069_rpc_type tr069_control_pick_request(tr069_control_mgr *control_handle);

INT32_T tr069_control_clear_task(tr069_control_mgr *control_handle);

INT32_T tr069_control_clear_request(tr069_control_mgr *control_handle);

INT32_T tr069_control_put_inform_parameter(tr069_control_mgr *control_handle, INT8_T *param_name);

INT32_T tr069_control_get_inform_parameters(tr069_control_mgr *control_handle, tr069_inform_parameters *parameters_recver);

INT32_T tr069_control_clear_inform_parameters(tr069_control_mgr *control_handle);

INT32_T tr069_control_get_event_number(tr069_event *event);

INT32_T tr069_control_is_event_empty(tr069_event *event);

INT32_T tr069_control_main_task(tr069_control_mgr *control_handle);

INT32_T tr069_control_ping();

INT32_T tr069_control_tracert();

INT32_T tr069_control_check_first_boot();

INT32_T tr069_control_speed_test();


#ifdef __cplusplus
}
#endif

#endif
