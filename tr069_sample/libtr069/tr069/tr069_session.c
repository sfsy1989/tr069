/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: Session
	Function: Session Management with ACS
********************************************************************/
#include <n_lib.h>

#include "tr069_session.h"
#include "tr069_control.h"
#include "tr069_service.h"
#include "tr069_interface.h"
#include "tr069_rpc.h"
#include "tr069_cookie.h"
#include "tr069.h"

/* global */

/* extern */

/* local */

tr069_session_mgr* tr069_session_init(tr069_mgr *tr069_handle)
{
	tr069_session_mgr *manager = NULL;

	tr069_debug("enter\n");
	if (NULL == tr069_handle)
	{
		tr069_error("tr069 handle error\n");
		goto ERROR_EXIT;
	}

	/* initialize session manager */
	manager = malloc(sizeof(tr069_session_mgr));
	if (NULL == manager)
	{
		tr069_error("malloc session manager error\n");
		goto ERROR_EXIT;
	}
	memset(manager, 0, sizeof(tr069_session_mgr));

	/* initialize all elements */
	manager->tr069_handle = tr069_handle;
	manager->status = TR069_SESSION_STATUS_STOP;
	manager->hold_request = 0;

	tr069_debug("ok quit\n");
	return manager;

ERROR_EXIT:

	/* if session manger already initialized, uninitialize it */
	if (NULL != manager)
	{
		manager = tr069_session_uninit(manager);
	}

	tr069_error("error quit\n");
	return NULL;
}

tr069_session_mgr* tr069_session_uninit(tr069_session_mgr *session_handle)
{
	tr069_debug("enter\n");
	if (NULL != session_handle)
	{
		/* uninitialize all elements */
		free(session_handle);
	}

	tr069_debug("ok quit\n");
	return NULL;
}

INT32_T tr069_session_clear(tr069_session_mgr *session_handle)
{
	tr069_debug("enter\n");
	if (NULL == session_handle)
	{
		tr069_error("session handle error\n");
		return -1;
	}

	/* clear */
	session_handle->status = TR069_SESSION_STATUS_STOP;
	session_handle->hold_request = 0;

	tr069_debug("ok quit\n");
	return 0;
}

INT32_T tr069_session_main(tr069_session_mgr *session_handle, VOID_T *void_event, VOID_T *void_inform_parameters)
{
	INT32_T ret;
	tr069_mgr *tr069_handle;
	tr069_interface_mgr *interface_handle;
	tr069_control_mgr *control_handle;
	tr069_rpc_mgr *rpc_handle = NULL;
	tr069_rpc_type request;
	tr069_task_type current_task;
	tr069_event *event = (tr069_event*)void_event;
	tr069_inform_parameters *inform_parameters = (tr069_inform_parameters*)void_inform_parameters;
	static INT8_T target_url[F_MAX_URL_SIZE] = {0};
	static INT8_T* ftp_s = "ftp://";

	if (NULL == session_handle)
	{
		tr069_error("session handle error\n");
		goto FINISH_EXIT;
	}

	if (NULL == event)
	{
		tr069_error("event pointer error\n");
		goto FINISH_EXIT;
	}

	if (NULL == inform_parameters)
	{
		tr069_error("inform parameters pointer error\n");
		goto FINISH_EXIT;
	}

	tr069_handle = session_handle->tr069_handle;
	if (NULL == tr069_handle)
	{
		tr069_error("tr069 handle error\n");
		goto FINISH_EXIT;
	}

	interface_handle = tr069_handle->interface_handle;
	if (NULL == interface_handle)
	{
		tr069_error("interface handle error\n");
		goto FINISH_EXIT;
	}

	control_handle = tr069_handle->control_handle;
	if (NULL == control_handle)
	{
		tr069_error("control handle error\n");
		goto FINISH_EXIT;
	}

	rpc_handle = tr069_handle->rpc_handle;
	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		goto FINISH_EXIT;
	}
	tr069_rpc_clear(rpc_handle);

	session_handle->status = TR069_SESSION_STATUS_INFORM;
	session_handle->set_param_flag = 0;
	
	while (1)
	{
		/* switch status to decide actions */
		switch (session_handle->status)
		{
		case TR069_SESSION_STATUS_INFORM:
			/* send inform request and receive inform response */
			if (1 == event->event_flag[TR069_EVENT_CONNECTION_REQUEST]) {
				tr069_clear_cookie(rpc_handle);
				tr069_set_req_cookie(rpc_handle);
			}

			ret = tr069_rpc_build_inform(rpc_handle, event, inform_parameters, 0);
			if (ret != 0)
			{
				tr069_error("build inform error\n");
				goto FINISH_EXIT;
			}

			ret = tr069_rpc_send(rpc_handle);
			if (ret != 0)
			{
				tr069_warning("send inform error\n");
				goto FINISH_EXIT;
			}

			ret = tr069_rpc_deal_recv(rpc_handle, rpc_handle->recv_buff.data);
			if (1 == event->event_flag[TR069_EVENT_CONNECTION_REQUEST]) {
				tr069_clr_req_cookie(rpc_handle);
			}

			if (0 == ret)
			{
				/* inform affirmed, switch to request status */
				tr069_debug("inform affirmed\n");
				session_handle->status = TR069_SESSION_STATUS_REQUEST;
				break;
			}
			else if (TR069_FAULT_ACS_RETRY_REQUEST == ret)
			{
				/* resend request */
				tr069_debug("acs require retry\n");
				break;
			}
			else
			{
				/* session break */
				tr069_warning("inform response error\n");
				goto FINISH_EXIT;
			}
			break;

		case TR069_SESSION_STATUS_REQUEST:
			/* send other request and receive other response */
			request = tr069_control_pick_request(control_handle);
			if (TR069_RPC_ACS_TRANSFER_COMPLETE == request)
			{
				/* send transfer complete request */
				ret = tr069_rpc_build_transfer_complete(rpc_handle,
									control_handle->event.command_key,
									control_handle->transfer_stime,
									control_handle->transfer_ctime,
									control_handle->transfer_result);
				if (ret != 0)
				{
					tr069_error("build transfer complete error\n");
					goto FINISH_EXIT;
				}
			}
			else
			{
				/* no more request, should send empty */
				ret = tr069_rpc_build_empty(rpc_handle);
				if (ret != 0)
				{
					tr069_error("build empty error\n");
					goto FINISH_EXIT;
				}
			}

			tr069_debug("start sending request\n");
			ret = tr069_rpc_send(rpc_handle);
			if (ret != 0)
			{
				tr069_error("send request error\n");
				goto FINISH_EXIT;
			}

			if (-1 == request)
			{
				/* empty sent, switch to response status */
				tr069_debug("empty sent\n");
				session_handle->status = TR069_SESSION_STATUS_RESPONSE;
				break;
			}
			else
			{
				/* should deal with response from acs */
				/* check for more request */
				tr069_debug("request sent\n");
				break;
			}
			break;

		case TR069_SESSION_STATUS_RESPONSE:
			/* receive rpc request and send rpc response */
#if 0
if (0 == rpc_handle->recv_buff.number)
			{
				/* receive empty means acs have no more request */
				if (0 == session_handle->hold_request)
				{
					/* session end */
					tr069_debug("[xiaoniu]===>acs send empty without hold request\n");
					goto FINISH_EXIT;
				}
				else
				{
					/* acs send empty with hold request, switch to execute status */
					tr069_debug("[xiaoniu]===>acs send empty with hold request\n");
					session_handle->status = TR069_SESSION_STATUS_EXECUTE;
					break;
				}
			}
			else
#endif				
				{
				/* receive rpc from acs */
				ret = tr069_rpc_deal_recv(rpc_handle, rpc_handle->recv_buff.data);

				if (0 == ret)
				{
					/* rpc deal ok, send back response */
					ret = tr069_rpc_send(rpc_handle);
					if (ret != 0)
					{
						tr069_error("send response error\n");
						goto FINISH_EXIT;
					}
					session_handle->status = TR069_SESSION_STATUS_EXECUTE;
					tr069_debug("response sent\n");
					break;
				}
				else if (-1 == ret)
				{
					/* fatal error in rpc dealing, session break */
					goto FINISH_EXIT;
				}
				else
				{
					/* fault in rpc dealing, send back fault */
					tr069_error("deal rpc fault\n");
					ret = tr069_rpc_send(rpc_handle);
					if (ret != 0)
					{
						tr069_error("send fault error\n");
						goto FINISH_EXIT;
					}

					tr069_debug("fault sent\n");
					break;
				}
			}
			break;

		case TR069_SESSION_STATUS_EXECUTE:
			/* if acs send rpc with "hold request", session should execute rpc */
			/* check for task queue */
			current_task = tr069_control_pick_task(control_handle);
			if (-1 == current_task)
			{
				/* no more task to execute, switch back to request status */
				tr069_debug("no more task, back to request\n");
				session_handle->hold_request = 0;
				session_handle->status = TR069_SESSION_STATUS_REQUEST;
				break;
			}
			else
			{
				/* switch task to execute */
				switch (current_task)
				{
				case TR069_TASK_REBOOT:
					/* reboot task */
					/* execute reboot here */
					tr069_debug("I'm executing reboot\n");
					//n_device_reboot();
					
					//system("busybox kill -3 `busybox pidof com.fonsview.tr069`");
					//system("busybox kill -2 `busybox pidof com.fonsview.config`");
					system("am broadcast -a android.intent.action.REBOOT --ei nowait 1 --ei interval 1 --ei window 0");
					break;

				case TR069_TASK_FACTORY_RESET:
					/* factory reset task */
					/* execute factory reset here */
					tr069_debug("I'm executing factory reset\n");
					n_cfg_factory_reset();
					tr069_service_set_status(TR069_STATUS_FIRSTBOOT, "1");
					n_device_reboot();
					break;

				case TR069_TASK_DOWNLOAD:
					/* download task */
					/* execute download here */
					tr069_debug("I'm executing download\n");
					
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
					//n_brows_tr069_upgrade(control_handle->download_argument.url,
					//		control_handle->download_argument.target_file_name);
					char cmd[N_MAX_SHELL_CMD_SIZE];
					snprintf(cmd, sizeof(cmd), "am broadcast -a com.fonsview.stb.OTA_ACTION --es PackageUrl %s --ei ForceType 0",target_url);
					system(cmd);
					break;

				case TR069_TASK_UPLOAD:
					/* upload task */
					/* execute upload here */
					tr069_debug("I'm executing upload\n");

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
					break;

				case TR069_TASK_PING:
					/* ping task */
					/* execute ping here */
					tr069_error("I'm executing ping\n");
					ret = tr069_control_ping();
					if (0 == ret) 
					{
						/* should trigger diagnostics complete event */
						tr069_control_set_event(control_handle, TR069_EVENT_DIAGNOSTICS_COMPLETE, 1, NULL);
						tr069_control_put_task(control_handle, TR069_TASK_SESSION);
					}
					break;

				case TR069_TASK_TRACERT:
					/* trace route task */
					/* execute trace route here */
					tr069_debug("I'm executing trace route\n");
					ret = tr069_control_tracert();
					
					if (ret == 0) 
					{
						/* should trigger diagnostics complete event */
						tr069_control_set_event(control_handle, TR069_EVENT_DIAGNOSTICS_COMPLETE, 1, NULL);
						tr069_control_put_task(control_handle, TR069_TASK_SESSION);
					}
					break;
					
				case TR069_TASK_SPEED_TEST:
					tr069_debug("I'm executing speed test\n");
					/* speed test task */
					/* execute speed test here */
					ret = tr069_control_speed_test();
					break;

				default:
					/* invalid task or no task */
					break;
				}
			}
			break;

		default:
			break;
		}
	}

FINISH_EXIT:

	session_handle->status = TR069_SESSION_STATUS_STOP;
	if (session_handle->set_param_flag == 1) {
		tr069_save_all_config();
	}
	if (rpc_handle) {
		tr069_clear_cookie(rpc_handle);
	}

	tr069_debug("ok quit\n");
	return 0;
}
