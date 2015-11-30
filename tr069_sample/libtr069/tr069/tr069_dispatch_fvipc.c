/* A simple standalone XML-RPC server written in C. */

#include <stdlib.h>
#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <n_lib.h>
#include "tr069_service.h"
#include "tr069_dispatch.h"
#include "tr069_interface.h"
#include "tr069.h"


int  tr069_dps_start(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_start.\n");

	int ret_i;

	tr069_debug("\n");
	ret_i = tr069_service_start();

        ipc_form_valueset(ret, "i", ret_i);
	return FV_OK;
}

int  tr069_dps_stop(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_stop.\n");

	int ret_i;

	tr069_debug("\n");
	ret_i = tr069_service_stop();
	if(0 != ret_i){
		tr069_error("tr069 service stop error!\n");
	}
	
	ipc_form_valueset(ret, "i", ret_i);
	return FV_OK;
}

int  tr069_dps_set_event(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_set_event.\n");

	int event_index;
	int event_value;
	int ret_i;
       char command_key[F_IPC_MAX_VALUESET_SIZE] = {0};

	ipc_deform_valueset(arg, "iis", &event_index, &event_value, command_key);
	tr069_debug("event_index:%d event_value:%d command_key:%s\n", event_index, event_value, command_key);
	ret_i = tr069_service_set_event(event_index, event_value, command_key);

	 ipc_form_valueset(ret, "i", ret_i);
	return FV_OK;
}

int  tr069_dps_put_request(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_put_request.\n");

	int request;
	int ret_i;

	ipc_deform_valueset(arg, "i", &request);
	tr069_debug("request:%d\n", request);
	ret_i = tr069_service_put_request(request);

	 ipc_form_valueset(ret, "i", ret_i);
	return FV_OK;
}

int  tr069_dps_put_inform_parameter(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_put_inform_parameter.\n");

	char param_name[F_IPC_MAX_VALUESET_SIZE] = {0};
	int ret_i;

	ipc_deform_valueset(arg, "s", param_name);
	tr069_debug("param_name:%s\n", param_name);
	ret_i = tr069_service_put_inform_parameter(param_name);

	ipc_form_valueset(ret, "i", ret_i);
	return FV_OK;
}

int tr069_dps_get_firstboot(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message tr069_dps_get_firstboot.\n");

	char *value = NULL;
	int retval;

	retval = tr069_service_get_status(TR069_STATUS_FIRSTBOOT, &value);
	if (FV_OK != retval) {
		tr069_error("tr069_service_get_status failed.\n");
		return -1;
	}

	ipc_form_valueset(ret, "s", value);
	free(value);

	return FV_OK;
}

int tr069_dps_set_firstboot(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message tr069_dps_set_firstboot.\n");
	char value[F_IPC_MAX_VALUESET_SIZE] = {0};
	int retval;

	ipc_deform_valueset(arg, "s", value);

	retval = tr069_service_set_status(TR069_STATUS_FIRSTBOOT, value);
	if (retval != FV_OK) {
		tr069_error("tr069_service_set_status failed.\n");
		return -1;
	}

	return FV_OK;
}

int  tr069_dps_get_config(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_get_config.\n");

	char name[F_IPC_MAX_VALUESET_SIZE] = {0};
        char *value=NULL;
	int ret_i;

	ipc_deform_valueset(arg, "s", name);
	tr069_debug("name:%s\n", name);
	ret_i = tr069_service_get_status(name, &value);

	ipc_form_valueset(ret, "is", ret_i, value);
        free(value);
	return FV_OK;
}

int  tr069_dps_set_config(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_set_config.\n");

	char name[F_IPC_MAX_VALUESET_SIZE] = {0};
        char value[F_IPC_MAX_VALUESET_SIZE] = {0};
	int ret_i;

	ipc_deform_valueset(arg, "ss", name, value);
	tr069_debug("name:%s value:%s\n", name, value);
	ret_i = tr069_service_set_status(name, value);

	ipc_form_valueset(ret, "i", ret_i);
	return FV_OK;
}

int  tr069_dps_set_transfer_complete(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_set_transfer_complete.\n");

	char start_time[F_IPC_MAX_VALUESET_SIZE] = {0};
        char complete_time[F_IPC_MAX_VALUESET_SIZE] = {0};

	int transfer_result;
	int ret_i;

	ipc_deform_valueset(arg, "ssi", start_time, complete_time, &transfer_result);
	tr069_debug("start_time:%s complete_time:%s transfer_result:%d\n", start_time, complete_time, transfer_result);
	ret_i = tr069_service_set_transfer_complete(start_time, complete_time, transfer_result);

        ipc_form_valueset(ret, "i", ret_i);
	return FV_OK;
}

int tr069_dps_cu_usb_config(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_cu_usb_config.\n");

	int ret_i;

	ret_i = tr069_service_cu_usb_config();

	ipc_form_valueset(ret, "i", ret_i);
	
	return FV_OK;
}

int tr069_dps_offline_notify(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_offline_notify.\n");

	int ret_i;

	tr069_debug("\n");
	ret_i = tr069_offline_notify();

        ipc_form_valueset(ret, "i", ret_i);
	return FV_OK;
}

int  tr069_dps_value_change_notify(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_set_notify.\n");

	char param[F_CFG_NUM + 1];
	int ret_i;

	ipc_deform_valueset(arg, "s", param);
	ret_i = tr069_value_change_notify(param);
	ipc_form_valueset(ret, "i", ret_i);
	
	return FV_OK;
}

int tr069_dps_alarm_report(ipc_valueset *arg, ipc_valueset *ret)
{
	tr069_debug("deal with message cmd_tr069_alarm_report.\n");

	int ret_i;
	ret_i = tr069_alarm_report();

        ipc_form_valueset(ret, "i", ret_i);
	
	return FV_OK;
}

int tr069_dispatch_init()
{
	ipc_service *service = NULL;

	service = n_init_service(N_MOD_CPE, NULL, 1);
	f_assert(NULL != service);
	if (NULL == service) {
		return -1;
	}

	ipc_add_interface(service, cmd_tr069_start, tr069_dps_start);
	ipc_add_interface(service, cmd_tr069_stop, tr069_dps_stop);
	ipc_add_interface(service, cmd_tr069_set_event, tr069_dps_set_event);
	ipc_add_interface(service, cmd_tr069_put_inform_parameter, tr069_dps_put_inform_parameter);
	ipc_add_interface(service, cmd_tr069_get_firstboot_flag, tr069_dps_get_firstboot);
	ipc_add_interface(service, cmd_tr069_set_firstboot_flag, tr069_dps_set_firstboot);
	ipc_add_interface(service, cmd_tr069_set_transfer_complete , tr069_dps_set_transfer_complete);
	ipc_add_interface(service, cmd_tr069_put_request, tr069_dps_put_request);
	ipc_add_interface(service, cmd_tr069_cu_usb_config, tr069_dps_cu_usb_config);
	ipc_add_interface(service, cmd_tr069_offline_notify, tr069_dps_offline_notify);
	ipc_add_interface(service, cmd_tr069_value_change_notify, tr069_dps_value_change_notify);
	ipc_add_interface(service, cmd_tr069_alarm_report, tr069_dps_alarm_report);

	return FV_OK;
}

void tr069_dispatch_uninit()
{
	return;
}

int tr069_dispatch_start()
{
	n_fvipc_start(F_IPC_TIMER_IDLE);
	return FV_OK;
}

int tr069_dispatch_stop()
{
	n_fvipc_stop();
	return FV_OK;
}

