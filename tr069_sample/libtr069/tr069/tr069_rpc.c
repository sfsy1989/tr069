/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: RPC
	Function: RPC support for tr069
********************************************************************/
/* fixme: should add hold request detection */
#include <n_lib.h>
#include "tr069_rpc.h"
#include "tr069_control.h"
#include "tr069_parameter.h"
#include "tr069_interface.h"
#include "tr069_session.h"
#include "tr069_timer.h"
#include "tr069.h"

/* global */

/* extern */
extern int tr069_set_config(int key, char *value);
extern int tr069_get_config(int key, char *value, unsigned int size);

extern INT8_T* tr069_parameter_name_table[TR069_PARAM_NUM];
int g_speedtest_flag = 0;
int g_packetlost_flag = 0;
/* local */
static INT32_T tr069_rpc_set_parameter_values(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_set_parameter_attributes(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_get_parameter_values(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_get_parameter_names(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_get_parameter_attributes(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_download(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_upload(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_reboot(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_factory_reset(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_x_fv_setup(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_unknown_rpc(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_deal_fault(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_inform_response(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);

static INT32_T tr069_rpc_transfer_complete_response(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap);


tr069_rpc_mgr* tr069_rpc_init(tr069_mgr *tr069_handle)
{
	tr069_rpc_mgr *manager = NULL;

	tr069_debug("enter\n");
	if (NULL == tr069_handle)
	{
		tr069_error("tr069 handle error\n");
		goto ERROR_EXIT;
	}

	manager = malloc(sizeof(tr069_rpc_mgr));
	if (NULL == manager)
	{
		tr069_error("malloc rpc manager error\n");
		goto ERROR_EXIT;
	}
	memset(manager, 0, sizeof(tr069_rpc_mgr));

	/* initialize all elements */
	manager->tr069_handle = tr069_handle;
	manager->xml_buffer = tr069_libxml_create();
	if (NULL == manager->xml_buffer)
	{
		tr069_error("xml buffer create fail\n");
		goto ERROR_EXIT;
	}

	manager->req_from_acs = TR069_RPC_NUMBER;
	manager->req_to_acs = TR069_RPC_NUMBER;

	manager->fault_soap = NULL;
	manager->fault_code = TR069_FAULT_NO_FAULT;
	manager->fault_node = NULL;

	tr069_debug("ok quit\n");
	return manager;

ERROR_EXIT:

	/* if rpc manager already initialized, uninitialize it */
	if (manager != NULL)
	{
		manager = tr069_rpc_uninit(manager);
	}

	tr069_error("error quit\n");
	return NULL;
}


tr069_rpc_mgr* tr069_rpc_uninit(tr069_rpc_mgr *rpc_handle)
{
	tr069_debug("enter\n");
	if (rpc_handle != NULL)
	{
		/* uninitialize all elements */
		if (rpc_handle->xml_buffer != NULL)
		{
			tr069_libxml_free(rpc_handle->xml_buffer);
			rpc_handle->xml_buffer = NULL;
		}

		if (rpc_handle->fault_soap != NULL)
		{
			tr069_libsoap_free(rpc_handle->fault_soap);
			rpc_handle->fault_soap = NULL;
		}

		rpc_handle->fault_code = TR069_FAULT_NO_FAULT;
		rpc_handle->fault_node = NULL;

		rpc_handle->req_from_acs = TR069_RPC_NUMBER;
		rpc_handle->req_to_acs = TR069_RPC_NUMBER;

		free(rpc_handle);
	}

	tr069_debug("ok quit\n");
	return NULL;
}


VOID_T tr069_rpc_clear(tr069_rpc_mgr *rpc_handle)
{
	tr069_debug("enter\n");
	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		return;
	}

	tr069_rpc_clear_fault(rpc_handle);

	rpc_handle->req_from_acs = TR069_RPC_NUMBER;
	rpc_handle->req_to_acs = TR069_RPC_NUMBER;
	rpc_handle->recv_buff.number = 0;

	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_debug("ok quit\n");
	return;
}


VOID_T tr069_rpc_clear_fault(tr069_rpc_mgr *rpc_handle)
{
	tr069_debug("enter\n");
	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		return;
	}

	if (rpc_handle->fault_soap != NULL)
	{
		tr069_libsoap_free(rpc_handle->fault_soap);
		rpc_handle->fault_soap = NULL;
	}

	rpc_handle->fault_code = TR069_FAULT_NO_FAULT;
	rpc_handle->fault_node = NULL;

	tr069_debug("ok quit\n");
	return;
}

/*****************************************************************************************
Function:
	build and inform soap struct and dump it into xml buffer

Input:
	rpc_handle: handle to rpc module involved
	event: event to inform
	inform_parameters: parameters should be informed
	retry_count: retry count

Output:

Return:
	-1: build fail
	0: build success
*****************************************************************************************/
INT32_T tr069_rpc_build_inform(tr069_rpc_mgr *rpc_handle, VOID_T *void_event, VOID_T *void_inform_parameters, UINT32_T retry_count)
{
	INT8_T string_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	INT8_T *method_name = "cwmp:Inform";
	SoapEnv* inform_soap = NULL;
	xmlNodePtr xml_node = NULL;
	tr069_parameter_mgr *param_handle = NULL;
	tr069_parameter *temp_param = NULL;
	time_t time_now;
	INT32_T i;
	tr069_event *event = (tr069_event*)void_event;
	tr069_inform_parameters *inform_parameters = (tr069_inform_parameters*)void_inform_parameters;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	if (NULL == event || tr069_control_is_event_empty(event) != 0)
	{
		tr069_error("event error\n");
		return -1;
	}

	if (NULL == inform_parameters)
	{
		tr069_error("inform parameters error\n");
		return -1;
	}

	param_handle = rpc_handle->tr069_handle->parameter_handle;
	if (NULL == param_handle)
	{
		tr069_error("parameter handle error\n");
		return -1;
	}


	tr069_libsoap_method_new(TR069_URN, method_name, &inform_soap);

	xml_node = tr069_libsoap_get_method(inform_soap);

	tr069_libsoap_push_item(inform_soap, "DeviceId", "xsi:type", "cwmp:DeviceIdStruct");

	tr069_parameter_get_param(param_handle,  tr069_parameter_name_table[TR069_PARAM_Manufacturer], &temp_param, 0, 0);
	tr069_parameter_get_value(param_handle, temp_param, string_buff, TR069_MAX_PARAM_VALUE_LEN);
	tr069_libsoap_add_item(inform_soap, "Manufacturer", string_buff, NULL, NULL);	
	
	memset(string_buff, 0, TR069_MAX_PARAM_VALUE_LEN);
	tr069_parameter_get_param(param_handle, tr069_parameter_name_table[TR069_PARAM_ManufacturerOUI], &temp_param, 0, 0);
	tr069_parameter_get_value(param_handle, temp_param, string_buff, TR069_MAX_PARAM_VALUE_LEN);
	tr069_libsoap_add_item(inform_soap, "OUI", string_buff, NULL, NULL);

	memset(string_buff, 0, TR069_MAX_PARAM_VALUE_LEN);
	tr069_parameter_get_param(param_handle,  tr069_parameter_name_table[TR069_PARAM_ProductClass], &temp_param, 0, 0);
	tr069_parameter_get_value(param_handle, temp_param, string_buff, TR069_MAX_PARAM_VALUE_LEN);
	tr069_libsoap_add_item(inform_soap, "ProductClass", string_buff, NULL, NULL);

	memset(string_buff, 0, TR069_MAX_PARAM_VALUE_LEN);
	tr069_parameter_get_param(param_handle, tr069_parameter_name_table[TR069_PARAM_SerialNumber], &temp_param, 0, 0);
	tr069_parameter_get_value(param_handle, temp_param, string_buff, TR069_MAX_PARAM_VALUE_LEN);
	tr069_libsoap_add_item(inform_soap, "SerialNumber", string_buff, NULL, NULL);
	tr069_libsoap_pop_item(inform_soap);

	sprintf(string_buff, "cwmp:EventStruct[%d]", event->number);
	tr069_libsoap_push_item(inform_soap, "Event", "soap:arrayType", string_buff);

	for (i = TR069_EVENT_BOOTSTRAP; i < TR069_EVENT_NUMBER; i++)
	{
		if (1 == event->event_flag[i])
		{
			/* an event has been triggered */
			tr069_libsoap_push_item(inform_soap, "EventStruct", NULL, NULL);

			tr069_libsoap_add_item(inform_soap, "EventCode", tr069_event_type_name(i), NULL, NULL);
			if (TR069_EVENT_M_REBOOT == i || TR069_EVENT_M_DOWNLOAD == i || TR069_EVENT_M_UPLOAD == i || TR069_EVENT_M_SCHEDULEINFORM == i)
			{
				tr069_libsoap_add_item(inform_soap, "CommandKey", event->command_key, NULL, NULL);
			}
			else
			{
				tr069_libsoap_add_item(inform_soap, "CommandKey", "", NULL, NULL);;
			}

			tr069_libsoap_pop_item(inform_soap);
		}
	}

	tr069_libsoap_pop_item(inform_soap);

	tr069_libsoap_add_item(inform_soap, "MaxEnvelopes", "1", NULL, NULL);

	time_now = time(NULL);
	tr069_format_time(time_now, string_buff, sizeof(string_buff));
	tr069_libsoap_add_item(inform_soap, "CurrentTime", string_buff, NULL, NULL);

	sprintf(string_buff, "%d", retry_count);
	tr069_libsoap_add_item(inform_soap, "RetryCount", string_buff, NULL, NULL);

	if (inform_parameters->number > 0)
	{
	xml_node = tr069_libsoap_push_item(inform_soap, "ParameterList", NULL, NULL);

	for (i = 0; i < inform_parameters->number; i++)
	{
		tr069_libsoap_push_item(inform_soap, "ParameterValueStruct", NULL, NULL);

		memset(string_buff, 0, TR069_MAX_PARAM_VALUE_LEN);
		tr069_parameter_get_param(param_handle, inform_parameters->parameters[i], &temp_param, 0, 0);
		tr069_parameter_get_value(param_handle, temp_param, string_buff, TR069_MAX_PARAM_VALUE_LEN);
		tr069_libsoap_add_item(inform_soap, "Name", inform_parameters->parameters[i], NULL, NULL);
		tr069_libsoap_add_item(inform_soap, "Value", string_buff, NULL, NULL);
		tr069_libsoap_pop_item(inform_soap);
	}

	tr069_libsoap_pop_item(inform_soap);

	sprintf(string_buff, "cwmp:ParameterValueStruct[%d]", i);
	tr069_libsoap_add_prop(xml_node, "soap:arrayType", string_buff);
	}

	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, inform_soap);
	
	tr069_libsoap_free(inform_soap);

	rpc_handle->req_to_acs = TR069_RPC_ACS_INFORM;

	return 0;
}

/*****************************************************************************************
Function:
	build and empty soap struct and dump it into xml buffer

Input:
	rpc_handle: handle to rpc module involved

Output:

Return:
	-1: build fail
	0: build success
*****************************************************************************************/
INT32_T tr069_rpc_build_empty(tr069_rpc_mgr *rpc_handle)
{
	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	tr069_libxml_empty(rpc_handle->xml_buffer);

	rpc_handle->req_to_acs = TR069_RPC_EMPTY;

	return 0;
}

/*****************************************************************************************
Function:
	build and transfer complete soap struct and dump it into xml buffer

Input:
	rpc_handle: handle to rpc module involved
	command_key: command key of transfer
	start_time: start time of transfer
	complete_time: complete time of transfer
	result: result of transfer

Output:

Return:
	-1: build fail
	0: build success
*****************************************************************************************/
INT32_T tr069_rpc_build_transfer_complete(tr069_rpc_mgr *rpc_handle, INT8_T *command_key, INT8_T *start_time, INT8_T *complete_time, tr069_fault_type result)
{
	INT8_T string_buff[TR069_MAX_FAULT_CODE_LEN] = {0};
	INT8_T *method_name = "cwmp:TransferComplete";
	SoapEnv* transfer_complete_soap = NULL;
	xmlNodePtr xml_node = NULL;
	tr069_control_mgr *control_handle = NULL;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}
	
	control_handle = rpc_handle->tr069_handle->control_handle;
	if (NULL == control_handle)
	{
		tr069_error("control handle error\n");
		return -1;
	}

	if (NULL == command_key)
	{
		tr069_error("command key error\n");
		return -1;
	}

	if (NULL == start_time)
	{
		tr069_error("start time error\n");
		return -1;
	}

	if (NULL == complete_time)
	{
		tr069_error("complete time error\n");
		return -1;
	}

	/* build transfer complete soap strut */
	/* create soap struct with method name */
	tr069_libsoap_method_new(TR069_URN, method_name, &transfer_complete_soap);

	xml_node = tr069_libsoap_get_method(transfer_complete_soap);

	/* add command key */
	tr069_libsoap_add_item(transfer_complete_soap, "CommandKey", command_key, NULL, NULL);
	control_handle->event.command_key[0] = 0;

	/* add fault struct */
	tr069_libsoap_push_item(transfer_complete_soap, "FaultStruct", NULL, NULL);

	sprintf(string_buff, "%d", result);
	tr069_libsoap_add_item(transfer_complete_soap, "FaultCode", string_buff, NULL, NULL);
	tr069_libsoap_add_item(transfer_complete_soap, "FaultString", tr069_fault_code_name(result), NULL, NULL);

	tr069_libsoap_pop_item(transfer_complete_soap);

	/* add start time */
	tr069_libsoap_add_item(transfer_complete_soap, "StartTime", start_time, NULL, NULL);

	/* add complete time */
	tr069_libsoap_add_item(transfer_complete_soap, "CompleteTime", complete_time, NULL, NULL);

	/* dump soap struct into xml buffer */
	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, transfer_complete_soap);

	tr069_libsoap_free(transfer_complete_soap);

	rpc_handle->req_to_acs = TR069_RPC_ACS_TRANSFER_COMPLETE;

	return 0;
}


INT32_T tr069_rpc_send(tr069_rpc_mgr *rpc_handle)
{
	INT32_T ret;
	tr069_interface_mgr *interface_handle = NULL;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	interface_handle = rpc_handle->tr069_handle->interface_handle;
	if (NULL == interface_handle || NULL == interface_handle->curl_handle)
	{
		tr069_error("interface handle error\n");
		return -1;
	}

	/* set libcurl options */
	ret = tr069_rpc_set_libcurl(rpc_handle);
	if (ret != 0)
	{
		tr069_error("set libcurl options error\n");
		return -1;
	}

	/* use libcurl to implement send and recv */
	rpc_handle->recv_buff.number = 0;
	tr069_debug("perform send\n");
	ret = curl_easy_perform(interface_handle->curl_handle);
	rpc_handle->recv_buff.data[rpc_handle->recv_buff.number] = 0;
	if (0 == ret)
	{
		return 0;
	}
	else
	{
		tr069_warning("curl error=%d\n", ret);
		if (CURLE_OPERATION_TIMEDOUT == ret)
		{
			n_iptv_log(IET_SOFTWARE_FAIL, IFL_ERROR, "tr069 send error", STB_ERR_LOAD_SOAP_TIMEOUT);
		}
		return -1;
	}
}

INT32_T tr069_rpc_deal_recv(tr069_rpc_mgr *rpc_handle, INT8_T *recv_data)
{
	INT8_T string_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	SoapEnv *recv_soap = NULL;
	xmlNodePtr xml_node = NULL;
	INT32_T ret;

	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	if (NULL == recv_data)
	{
		tr069_error("recv_data null\n");
		return -1;
	}

	ret = tr069_libsoap_buffer_new(recv_data, &recv_soap);

	if (ret != 0)
	{
		tr069_warning("tr069_libsoap_buffer_new fail %d\n", ret);
		return -1;
	}

	if (NULL == recv_soap)
	{
		return -1;
	}

	xml_node = tr069_libsoap_get_method(recv_soap);
	if (NULL == xml_node)
	{
		tr069_error("tr069_libsoap_get_method fail %d\n", ret);
		tr069_libsoap_free(recv_soap);
		return -1;
	}
	if (TR069_RPC_EMPTY == rpc_handle->req_to_acs)
	{
		if (strcmp((char *)xml_node->name, tr069_rpc_type_name(TR069_RPC_CPE_SET_PARAMETER_VALUES)) == 0)
		{
			/* SetParameterValues */
			ret = tr069_rpc_set_parameter_values(rpc_handle, recv_soap);
		}
		else if (strcmp((char *)xml_node->name, tr069_rpc_type_name(TR069_RPC_CPE_GET_PARAMETER_VALUES)) == 0)
		{
			/* GetParameterValues */
			ret = tr069_rpc_get_parameter_values(rpc_handle, recv_soap);
		}
		else if (strcmp((char *)xml_node->name, tr069_rpc_type_name(TR069_RPC_CPE_GET_PARAMETER_NAMES)) == 0)
		{
			/* GetParameterNames */
			ret = tr069_rpc_get_parameter_names(rpc_handle, recv_soap);
		}
		else if (strcmp((char *)xml_node->name, tr069_rpc_type_name(TR069_RPC_CPE_SET_PARAMETER_ATTRIBUTES)) == 0)
		{
			/* SetParameterAttributes */
			ret = tr069_rpc_set_parameter_attributes(rpc_handle, recv_soap);
		}
		else if (strcmp((char *)xml_node->name, tr069_rpc_type_name(TR069_RPC_CPE_GET_PARAMETER_ATTRIBUTES)) == 0)
		{
			/* GetParameterAttributes */
			ret = tr069_rpc_get_parameter_attributes(rpc_handle, recv_soap);
		}
		else if (strcmp((char *)xml_node->name, tr069_rpc_type_name(TR069_RPC_CPE_DOWNLOAD)) == 0)
		{
			/* Download */
			ret = tr069_rpc_download(rpc_handle, recv_soap);
		}
		else if (strcmp((char *)xml_node->name, tr069_rpc_type_name(TR069_RPC_CPE_UPLOAD)) == 0)
		{
			/* Upload */
			ret = tr069_rpc_upload(rpc_handle, recv_soap);
		}
		else if (strcmp((char *)xml_node->name, tr069_rpc_type_name(TR069_RPC_CPE_REBOOT)) == 0)
		{
			/* Reboot */
			ret = tr069_rpc_reboot(rpc_handle, recv_soap);
		}
		else if (strcmp((char *)xml_node->name, tr069_rpc_type_name(TR069_RPC_CPE_FACTORY_RESET)) == 0)
		{
			/* FactoryReset */
			ret = tr069_rpc_factory_reset(rpc_handle, recv_soap);
		}
		else if (strcmp((char *)xml_node->name, tr069_rpc_type_name(TR069_RPC_CPE_X_FV_SETUP)) == 0)
		{
			/* X_FV_Setup */
			ret = tr069_rpc_x_fv_setup(rpc_handle, recv_soap);
		}
		else
		{
			ret = tr069_rpc_unknown_rpc(rpc_handle, recv_soap);
		}
	}
	else
	{
		if (strcmp((char *)xml_node->name, "Fault") == 0)
		{
			ret = tr069_rpc_deal_fault(rpc_handle, recv_soap);
		}
		else
		{
			sprintf(string_buff, "%s%s", tr069_rpc_type_name(rpc_handle->req_to_acs), "Response");
			if (strcmp((char *)xml_node->name, string_buff) != 0)
			{
				tr069_error("unexpected response\n");
				ret = -1;
			}
			else
			{
				if (TR069_RPC_ACS_INFORM == rpc_handle->req_to_acs)
				{
					/* InformResponse */
					ret = tr069_rpc_inform_response(rpc_handle, recv_soap);
				}
				else if (TR069_RPC_ACS_TRANSFER_COMPLETE == rpc_handle->req_to_acs)
				{
					/* TransferCompleteResponse */
					ret = tr069_rpc_transfer_complete_response(rpc_handle, recv_soap);
				}
				else
				{
					tr069_error("response not support\n");
					ret = -1;
				}
			}
		}
	}

	tr069_libsoap_free(recv_soap);

	return ret;
}


INT8_T* tr069_event_type_name(tr069_event_type event_type)
{
	INT8_T *event_type_name = NULL;

	switch (event_type)
	{
	case TR069_EVENT_BOOTSTRAP:
		event_type_name = "0 BOOTSTRAP";
		break;

	case TR069_EVENT_BOOT:
		event_type_name = "1 BOOT";
		break;

	case TR069_EVENT_PERIODIC:
		event_type_name = "2 PERIODIC";
		break;

	case TR069_EVENT_SCHEDULED:
		event_type_name = "3 SCHEDULED";
		break;

	case TR069_EVENT_VALUE_CHANGE:
		event_type_name = "4 VALUE CHANGE";
		break;

	case TR069_EVENT_KICKED:
		event_type_name = "5 KICKED";
		break;

	case TR069_EVENT_CONNECTION_REQUEST:
		event_type_name = "6 CONNECTION REQUEST";
		break;

	case TR069_EVENT_TRANSFER_COMPLETE:
		event_type_name = "7 TRANSFER COMPLETE";
		break;

	case TR069_EVENT_DIAGNOSTICS_COMPLETE:
		event_type_name = "8 DIAGNOSTICS COMPLETE";
		break;
		
	case TR069_EVENT_REQUEST_DOWNLOAD:
		event_type_name = "9 REQUEST DOWNLOAD";
		break;

	case TR069_EVENT_AUTONMOUS_TRANSFER_COMPLETE:
		event_type_name = "10 AUTONMOUS TRANSFER COMPLETE";
		break;

	case TR069_EVENT_X_CU_ALARM:
		event_type_name = "X CU Alarm";
		break;
		
	case TR069_EVENT_M_SCHEDULEINFORM:
		event_type_name = "M ScheduleInform";
		break;

	case TR069_EVENT_M_REBOOT:
		event_type_name = "M Reboot";
		break;

	case TR069_EVENT_M_DOWNLOAD:
		event_type_name = "M Download";
		break;

	case TR069_EVENT_M_UPLOAD:
		event_type_name = "M Upload";
		break;

	case TR069_EVENT_M_CTC_LOG_PERIODIC:
		event_type_name = "M CTC LOG_PERIODIC";
		break;

	default:
		break;
	}

	return event_type_name;
}

/*****************************************************************************************
Function:
	get file type name string of given file type

Input:
	file_type: the give file type

Output:

Return:
	NULL: get error
	other: pointer of name string
*****************************************************************************************/
INT8_T* tr069_file_type_name(tr069_file_type file_type)
{
	INT8_T *file_type_name = NULL;

	switch (file_type)
	{
	case TR069_DOWNLOAD_FILE_UPGRADE_IMAGE:
		file_type_name = "1 Firmware Upgrade Image";
		break;

	case TR069_DOWNLOAD_FILE_WEB_CONTENT:
		file_type_name = "2 Web Content";
		break;

	case TR069_DOWNLOAD_FILE_VENDOR_CONFIG:
		file_type_name = "3 Vendor Configuration File";
		break;

	case TR069_DOWNLOAD_FILE_VENDOR_LOG:
		file_type_name = "4 Vendor Log File";
		break;

	case TR069_UPLOAD_FILE_VENDOR_CONFIG:
		file_type_name = "1 Vendor Configuration File";
		break;

	case TR069_UPLOAD_FILE_VENDOR_LOG:
		file_type_name = "2 Vendor Log File";
		break;

	default:
		break;
	}

	return file_type_name;
}

/*****************************************************************************************
Function:
	get rpc type name string of given rpc type

Input:
	rpc_type: the give rpc type

Output:

Return:
	NULL: get error
	other: pointer of name string
*****************************************************************************************/
INT8_T* tr069_rpc_type_name(tr069_rpc_type rpc_type)
{
	INT8_T *rpc_type_name = NULL;

	switch (rpc_type)
	{
	case TR069_RPC_EMPTY:
		rpc_type_name = "Undefined";
		break;

	case TR069_RPC_ACS_INFORM:
		rpc_type_name = "Inform";
		break;

	case TR069_RPC_ACS_TRANSFER_COMPLETE:
		rpc_type_name = "TransferComplete";
		break;

	case TR069_RPC_CPE_SET_PARAMETER_VALUES:
		rpc_type_name = "SetParameterValues";
		break;

	case TR069_RPC_CPE_GET_PARAMETER_VALUES:
		rpc_type_name = "GetParameterValues";
		break;

	case TR069_RPC_CPE_GET_PARAMETER_NAMES:
		rpc_type_name = "GetParameterNames";
		break;

	case TR069_RPC_CPE_DOWNLOAD:
		rpc_type_name = "Download";
		break;

	case TR069_RPC_CPE_UPLOAD:
		rpc_type_name = "Upload";
		break;

	case TR069_RPC_CPE_REBOOT:
		rpc_type_name = "Reboot";
		break;

	case TR069_RPC_CPE_FACTORY_RESET:
		rpc_type_name = "FactoryReset";
		break;

	case TR069_RPC_CPE_SET_PARAMETER_ATTRIBUTES:
		rpc_type_name = "SetParameterAttributes";
		break;

	case TR069_RPC_CPE_GET_PARAMETER_ATTRIBUTES:
		rpc_type_name = "GetParameterAttributes";
		break;
	
	case TR069_RPC_CPE_X_FV_SETUP:
		rpc_type_name = "X_FV_Setup";
		break;

	default:
		break;
	}

	return rpc_type_name;
}


INT32_T tr069_rpc_inform_response(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	xmlNodePtr xml_node = NULL;
	INT8_T *str;

	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	xml_node = tr069_libsoap_get_method(recv_soap);

	xml_node = tr069_libsoap_get_child(xml_node);

	str = tr069_libsoap_get_content(xml_node);
	if (NULL == str)
	{
		return -1;
	}

	if (strcmp((char *)xml_node->name, "MaxEnvelopes") == 0 && strcmp(str, "1") == 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

/*****************************************************************************************
Function:
	deal with the transfer complete response

Input:
	rpc_handle: handle to rpc module involved
	recv_soap: soap struct of transfer complete response

Output:

Return:
	-1: response error
	0: response ok
*****************************************************************************************/
INT32_T tr069_rpc_transfer_complete_response(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	/* transfer complete response has no argument */
	return 0;
}

INT32_T tr069_rpc_deal_fault(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	xmlNodePtr xml_node = NULL;
	INT32_T fault_code;

	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	xml_node = tr069_libsoap_get_method(recv_soap);
	if (NULL == xml_node)
	{
		return -1;
	}

	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		return -1;
	}

	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		return -1;
	}

	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		return -1;
	}

	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		return -1;
	}

	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		return -1;
	}

	fault_code = atoi(tr069_libsoap_get_content(xml_node));

	if (TR069_FAULT_ACS_RETRY_REQUEST == fault_code)
	{
		return fault_code;
	}
	else
	{
		return -1;
	}
}


INT32_T tr069_rpc_unknown_rpc(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	INT32_T ret;
	INT32_T fault_code;

	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	fault_code = TR069_FAULT_CPE_METHOD_NOT_SUPPORTED;
	ret = tr069_rpc_build_fault(rpc_handle, fault_code);

	if (-1 == ret)
	{
		tr069_rpc_clear_fault(rpc_handle);
		return -1;
	}
	else
	{
		tr069_libxml_empty(rpc_handle->xml_buffer);

		tr069_libxml_dump(rpc_handle->xml_buffer, (SoapEnv*)(rpc_handle->fault_soap));

		tr069_rpc_clear_fault(rpc_handle);

		return fault_code;
	}
}


INT32_T tr069_rpc_factory_reset(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	INT8_T *method_name = "cwmp:FactoryResetResponse";
	SoapEnv* send_soap = NULL;
	xmlNodePtr xml_node = NULL;
	tr069_control_mgr *control_handle = NULL;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	control_handle = rpc_handle->tr069_handle->control_handle;
	if (NULL == control_handle)
	{
		tr069_error("control handle error\n");
		return -1;
	}

	/* should insert factory reset task to control manager */
	tr069_debug("receive factory reset\n");
	tr069_control_put_task(control_handle, TR069_TASK_FACTORY_RESET);

	tr069_libsoap_method_new(TR069_URN, method_name, &send_soap);

	xml_node = tr069_libsoap_get_method(send_soap);

	tr069_libsoap_add_item(send_soap, "Status", "0", NULL, NULL);

	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, send_soap);

	tr069_libsoap_free(send_soap);

	return 0;
}

INT32_T tr069_rpc_check_headers(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	xmlNodePtr xml_node = NULL;
	tr069_session_mgr *session_handle = NULL;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	session_handle = rpc_handle->tr069_handle->session_handle;
	if (NULL == session_handle)
	{
		tr069_error("session handle error\n");
		return -1;
	}

	/* get header of received soap */
	xml_node = tr069_libsoap_get_header(recv_soap);
	if (NULL == xml_node)
	{
		tr069_debug("received soap has no header node\n");
		return 0;
	}

	/* check all child nodes of header node */
	xml_node = tr069_libsoap_get_child(xml_node);
	while (xml_node != NULL)
	{
		if (strcmp("HoldRequests", (char *)xml_node->name) == 0)
		{
			/* hold request node found */
			if (strcmp("1", tr069_libsoap_get_content(xml_node)) == 0)
			{
				/* acs request hold requests */
				tr069_debug("acs require hold requests\n");
				//session_handle->hold_request = 1;
			}
		}

		xml_node = tr069_libsoap_get_next(xml_node);
	}

	return 0;
}

INT32_T tr069_rpc_reboot(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	INT8_T *method_name = "cwmp:RebootResponse";
	SoapEnv* send_soap = NULL;
	xmlNodePtr xml_node = NULL;
	tr069_control_mgr *control_handle = NULL;
	INT8_T *command_key = NULL;
	INT32_T ret;
	INT32_T fault_code;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	control_handle = rpc_handle->tr069_handle->control_handle;
	if (NULL == control_handle)
	{
		tr069_error("control handle error\n");
		return -1;
	}

	xml_node = tr069_libsoap_get_method(recv_soap);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* CommandKey */
	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("CommandKey", (char *)xml_node->name) != 0)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	command_key = tr069_libsoap_get_content(xml_node);
	if (NULL == command_key)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->event.command_key, command_key, sizeof(control_handle->event.command_key) - 1);

	/* should insert reboot task to control manager */
	tr069_control_put_task(control_handle, TR069_TASK_REBOOT);

	tr069_libsoap_method_new(TR069_URN, method_name, &send_soap);

	xml_node = tr069_libsoap_get_method(send_soap);

	tr069_libsoap_add_item(send_soap, "Status", "0", NULL, NULL);

	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, send_soap);

	tr069_libsoap_free(send_soap);

	return 0;

FAULT:

	if (-1 == ret)
	{
		tr069_rpc_clear_fault(rpc_handle);

		return -1;
	}
	else
	{
		tr069_libxml_empty(rpc_handle->xml_buffer);

		tr069_libxml_dump(rpc_handle->xml_buffer, (SoapEnv*)(rpc_handle->fault_soap));

		tr069_rpc_clear_fault(rpc_handle);

		return fault_code;
	}
}

/*****************************************************************************************
Function:
	deal with download request and build download response

Input:
	rpc_handle: handle to rpc module involved
	recv_soap: soap struct of download request

Output:

Return:
	-1: deal fail
	0: deal success
*****************************************************************************************/
INT32_T tr069_rpc_download(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	INT8_T *method_name = "cwmp:DownloadResponse";
	SoapEnv* send_soap = NULL;
	xmlNodePtr xml_node = NULL;
	tr069_control_mgr *control_handle = NULL;
	INT32_T ret;
	INT32_T fault_code;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	control_handle = rpc_handle->tr069_handle->control_handle;
	if (NULL == control_handle)
	{
		tr069_error("control handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	/* acquire rpc arguments */
	xml_node = tr069_libsoap_get_method(recv_soap);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* get command key */
	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("CommandKey", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 32)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->event.command_key,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->event.command_key) - 1);

	/* get file type */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("FileType", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 64)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->download_argument.file_type,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->download_argument.file_type) - 1);

	/* get url */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("URL", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 256)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->download_argument.url,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->download_argument.url) - 1);

	/* get username */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("Username", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 256)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->download_argument.username,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->download_argument.username) - 1);

	/* get password */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("Password", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 256)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->download_argument.password,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->download_argument.password) - 1);

	/* get file size */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("FileSize", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (atoi(tr069_libsoap_get_content(xml_node)) < 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	control_handle->download_argument.file_size = atoi(tr069_libsoap_get_content(xml_node));

	/* get target file name */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("TargetFileName", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 256)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->download_argument.target_file_name,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->download_argument.target_file_name) - 1);

	/* get delay seconds */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("DelaySeconds", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (atoi(tr069_libsoap_get_content(xml_node)) < 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	control_handle->download_argument.delay_seconds = atoi(tr069_libsoap_get_content(xml_node));

	/* get success url */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("SuccessURL", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 256)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->download_argument.success_url,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->download_argument.success_url) - 1);

	/* get failure url */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("FailureURL", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 256)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->download_argument.failure_url,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->download_argument.success_url) - 1);

	/* should insert download task to control manager */
	tr069_debug("receive download\n");
	tr069_control_put_task(control_handle, TR069_TASK_DOWNLOAD);

	/* build download response */
	tr069_libsoap_method_new(TR069_URN, method_name, &send_soap);
	if (NULL == send_soap)
	{
		/* fault 9002 internal error */
		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	xml_node = tr069_libsoap_get_method(send_soap);
	if (NULL == xml_node)
	{
		/* fault 9002 internal error */
		tr069_libsoap_free(send_soap);

		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* add status */
	tr069_libsoap_add_item(send_soap, "Status", "1", NULL, NULL);

	/* add start time */
	tr069_libsoap_add_item(send_soap, "StartTime", "", NULL, NULL);

	/* add complete time */
	tr069_libsoap_add_item(send_soap, "CompleteTime", "", NULL, NULL);

	/* dump soap struct into xml buffer */
	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, send_soap);

	tr069_libsoap_free(send_soap);

	return 0;

FAULT:

	if (-1 == ret)
	{
		tr069_rpc_clear_fault(rpc_handle);

		return -1;
	}
	else
	{
		tr069_libxml_empty(rpc_handle->xml_buffer);

		tr069_libxml_dump(rpc_handle->xml_buffer, (SoapEnv*)(rpc_handle->fault_soap));

		tr069_rpc_clear_fault(rpc_handle);

		return fault_code;
	}
}

/*****************************************************************************************
Function:
	deal with upload request and build upload response

Input:
	rpc_handle: handle to rpc module involved
	recv_soap: soap struct of upload request

Output:

Return:
	-1: deal fail
	0: deal success
*****************************************************************************************/
INT32_T tr069_rpc_upload(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	INT8_T *method_name = "cwmp:UploadResponse";
	SoapEnv* send_soap = NULL;
	xmlNodePtr xml_node = NULL;
	tr069_control_mgr *control_handle = NULL;
	INT32_T ret;
	INT32_T fault_code;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	control_handle = rpc_handle->tr069_handle->control_handle;
	if (NULL == control_handle)
	{
		tr069_error("control handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	/* acquire rpc arguments */
	xml_node = tr069_libsoap_get_method(recv_soap);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* get command key */
	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("CommandKey", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 32)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->event.command_key,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->event.command_key) - 1);

	/* get file type */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("FileType", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 64)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->upload_argument.file_type,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->upload_argument.file_type) - 1);

	/* get url */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("URL", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 256)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->upload_argument.url,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->upload_argument.url) - 1);

	/* get username */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("Username", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 256)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->upload_argument.username,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->upload_argument.username) - 1);

	/* get password */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("Password", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= 256)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(control_handle->upload_argument.password,
			tr069_libsoap_get_content(xml_node),
			sizeof(control_handle->upload_argument.password) - 1);

	/* get delay seconds */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("DelaySeconds", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (atoi(tr069_libsoap_get_content(xml_node)) < 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	control_handle->upload_argument.delay_seconds = atoi(tr069_libsoap_get_content(xml_node));

	/* should insert upload task to control manager */
	tr069_debug("receive upload\n");
	tr069_control_put_task(control_handle, TR069_TASK_UPLOAD);

	/* build upload response */
	tr069_libsoap_method_new(TR069_URN, method_name, &send_soap);
	if (NULL == send_soap)
	{
		/* fault 9002 internal error */
		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	xml_node = tr069_libsoap_get_method(send_soap);
	if (NULL == xml_node)
	{
		/* fault 9002 internal error */
		tr069_libsoap_free(send_soap);

		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* add status */
	tr069_libsoap_add_item(send_soap, "Status", "1", NULL, NULL);

	/* add start time */
	tr069_libsoap_add_item(send_soap, "StartTime", "", NULL, NULL);

	/* add complete time */
	tr069_libsoap_add_item(send_soap, "CompleteTime", "", NULL, NULL);

	/* dump soap struct into xml buffer */
	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, send_soap);

	tr069_libsoap_free(send_soap);

	return 0;

FAULT:

	if (-1 == ret)
	{
		tr069_rpc_clear_fault(rpc_handle);

		return -1;
	}
	else
	{
		tr069_libxml_empty(rpc_handle->xml_buffer);

		tr069_libxml_dump(rpc_handle->xml_buffer, (SoapEnv*)(rpc_handle->fault_soap));

		tr069_rpc_clear_fault(rpc_handle);

		return fault_code;
	}
}

/*****************************************************************************************
Function:
	deal with x_fv_setup request and build x_fv_setup response
	x_fv_setup method is vendor defined

Input:
	rpc_handle: handle to rpc module involved
	recv_soap: soap struct of x_fv_setup request

Output:

Return:
	-1: deal fail
	0: deal success
*****************************************************************************************/
INT32_T tr069_rpc_x_fv_setup(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	INT8_T *method_name = "cwmp:X_FV_SetupResponse";
	SoapEnv* send_soap = NULL;
	xmlNodePtr xml_node = NULL;
	tr069_control_mgr *control_handle = NULL;
	INT32_T ret;
	INT32_T status = 0;
	INT32_T fault_code;
	INT8_T mac[33] = {0};
	INT8_T stbid[33] = {0};

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	control_handle = rpc_handle->tr069_handle->control_handle;
	if (NULL == control_handle)
	{
		tr069_error("control handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	/* acquire rpc arguments */
	xml_node = tr069_libsoap_get_method(recv_soap);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* get mac */
	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("Mac", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) > 32)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(mac, tr069_libsoap_get_content(xml_node), sizeof(mac) - 1);

	/* get stbid */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("StbId", (char *)xml_node->name) != 0)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) > 32)
	{
		/* fault 9003 invalid arguments */
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strncpy(stbid, tr069_libsoap_get_content(xml_node), sizeof(stbid) - 1);

	/* should call int init_mac_stbid(char * mac,char *stbid); */
	tr069_debug("should call init_mac_stbid with mac: %s stbid: %s\n", mac, stbid);
	ret = tr069_set_config(TR069_PARAM_MACAddress, mac);
	if (ret != 0)
		status = 1;
	ret = tr069_set_config(TR069_PARAM_STBID, stbid);
	if (ret != 0)
		status = 1;

	/* build x_fv_setup response */
	tr069_libsoap_method_new(TR069_URN, method_name, &send_soap);
	if (NULL == send_soap)
	{
		/* fault 9002 internal error */
		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	xml_node = tr069_libsoap_get_method(send_soap);
	if (NULL == xml_node)
	{
		/* fault 9002 internal error */
		tr069_libsoap_free(send_soap);

		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* add status */
	tr069_libsoap_add_item(send_soap, "Status", 0 == status ? "0" : "1", NULL, NULL);

	/* dump soap struct into xml buffer */
	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, send_soap);

	tr069_libsoap_free(send_soap);

	return 0;

FAULT:

	if (-1 == ret)
	{
		tr069_rpc_clear_fault(rpc_handle);

		return -1;
	}
	else
	{
		tr069_libxml_empty(rpc_handle->xml_buffer);

		tr069_libxml_dump(rpc_handle->xml_buffer, (SoapEnv*)(rpc_handle->fault_soap));

		tr069_rpc_clear_fault(rpc_handle);

		return fault_code;
	}
}

INT32_T tr069_rpc_get_parameter_names(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	tr069_parameter *param_result[TR069_MAX_PARAM_NUM];
	INT8_T string_buff[TR069_MAX_PARAM_NAME_LEN] = {0};
	INT8_T *method_name = "cwmp:GetParameterNamesResponse";
	SoapEnv* send_soap = NULL;
	xmlNodePtr xml_node = NULL;
	tr069_parameter_mgr *param_handle = NULL;
	INT32_T i;
	INT32_T ret;
	INT32_T fault_code;
	UINT8_T next_level;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	param_handle = rpc_handle->tr069_handle->parameter_handle;
	if (NULL == param_handle)
	{
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	xml_node = tr069_libsoap_get_method(recv_soap);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* ParameterPath */
	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("ParameterPath", (char *)xml_node->name) != 0)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strlen(tr069_libsoap_get_content(xml_node)) >= TR069_MAX_PARAM_NAME_LEN)
	{
		fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}
	strcpy(string_buff, tr069_libsoap_get_content(xml_node));

	/* NextLevel */
	xml_node = tr069_libsoap_get_next(xml_node);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("NextLevel", (char *)xml_node->name) != 0)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	next_level = atoi(tr069_libsoap_get_content(xml_node));

	ret = tr069_parameter_get_param(param_handle, string_buff, param_result, next_level, 0);

	if (-1 == ret)
	{
		fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	tr069_libsoap_method_new(TR069_URN, method_name, &send_soap);
	if (NULL == send_soap)
	{
		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	xml_node = tr069_libsoap_get_method(send_soap);
	if (NULL == xml_node)
	{
		tr069_libsoap_free(send_soap);

		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	sprintf(string_buff, "cwmp:ParameterInfoStruct[%d]", ret);
	tr069_libsoap_push_item(send_soap, "ParameterList", "soap:arrayType", string_buff);

	for (i = 0; i < ret; i++)
	{
		tr069_libsoap_push_item(send_soap, "ParameterInfoStruct", NULL, NULL);

		tr069_libsoap_add_item(send_soap, "Name", param_result[i]->param_name, NULL, NULL);
		tr069_libsoap_add_item(send_soap, "Writable", 1 == param_result[i]->write_flag ? "True" : "False", NULL, NULL);

		tr069_libsoap_pop_item(send_soap);
	}

	tr069_libsoap_pop_item(send_soap);

	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, send_soap);

	tr069_libsoap_free(send_soap);

	return 0;

FAULT:

	if (-1 == ret)
	{
		tr069_rpc_clear_fault(rpc_handle);

		return -1;
	}
	else
	{
		tr069_libxml_empty(rpc_handle->xml_buffer);

		tr069_libxml_dump(rpc_handle->xml_buffer, (SoapEnv*)(rpc_handle->fault_soap));

		tr069_rpc_clear_fault(rpc_handle);

		return fault_code;
	}
}


INT32_T tr069_rpc_get_parameter_values(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	tr069_parameter *param_result[TR069_MAX_PARAM_NUM];
	INT8_T string_buff[TR069_MAX_PARAM_NAME_LEN] = {0};
	INT8_T value_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	INT8_T *method_name = "cwmp:GetParameterValuesResponse";
	SoapEnv* send_soap = NULL;
	xmlNodePtr xml_node = NULL;
	xmlNodePtr xml_back = NULL;
	tr069_parameter_mgr *param_handle = NULL;
	INT32_T param_num;
	INT32_T i;
	INT32_T ret;
	INT32_T fault_code;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	param_handle = rpc_handle->tr069_handle->parameter_handle;
	if (NULL == param_handle)
	{
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	xml_node = tr069_libsoap_get_method(recv_soap);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* ParameterNames */
	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("ParameterNames", (char *)xml_node->name) != 0)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	xml_node = tr069_libsoap_get_child(xml_node);

	tr069_libsoap_method_new(TR069_URN, method_name, &send_soap);
	if (NULL == send_soap)
	{
		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	xml_back = tr069_libsoap_get_method(send_soap);
	if (NULL == xml_back)
	{
		tr069_libsoap_free(send_soap);

		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	param_num = 0;
	xml_back = tr069_libsoap_push_item(send_soap, "ParameterList", NULL, NULL);

	while (xml_node != NULL)
	{
		if (strcmp("ParameterName", (char *)xml_node->name) != 0
		&& strcmp("string", (char *)xml_node->name) != 0)
		{
			tr069_libsoap_free(send_soap);

			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strlen(tr069_libsoap_get_content(xml_node)) >= TR069_MAX_PARAM_NAME_LEN)
		{
			tr069_libsoap_free(send_soap);

			fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}
		strcpy(string_buff, tr069_libsoap_get_content(xml_node));

		/* get parameters by parameter name */
		if (!strcmp(string_buff, "Device.LAN.DNSServers"))
		{
			/* dnsservers means "dnsserver0, dnsserver1" */
			ret = tr069_parameter_get_param(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer0Real], param_result, 0, 0);
			if (-1 == ret)
			{
				/* fault 9005 invalid parameter name*/
				tr069_libsoap_free(send_soap);
				fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
				ret = tr069_rpc_build_fault(rpc_handle, fault_code);

				goto FAULT;
			}
			ret = tr069_parameter_get_param(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer1Real], param_result + 1, 0, 0);
			if (-1 == ret)
			{
				/* fault 9005 invalid parameter name*/
				tr069_libsoap_free(send_soap);
				fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
				ret = tr069_rpc_build_fault(rpc_handle, fault_code);

				goto FAULT;
			}
			tr069_parameter_get_value(param_handle, param_result[0], value_buff, TR069_MAX_PARAM_VALUE_LEN);
			if (value_buff[0] != 0)
				strcat(value_buff, ",");
			tr069_parameter_get_value(param_handle, param_result[1], value_buff + strlen(value_buff), TR069_MAX_PARAM_VALUE_LEN);
			if (strlen(value_buff) > 0)
				if (value_buff[strlen(value_buff) - 1] == ',')
					value_buff[strlen(value_buff) - 1] = 0;

			param_num++;
			tr069_libsoap_push_item(send_soap, "ParameterValueStruct", NULL, NULL);

			tr069_libsoap_add_item(send_soap, "Name", string_buff, NULL, NULL);
			tr069_libsoap_add_item(send_soap, "Value", value_buff, "xsi:type", "xsd:string");

			tr069_libsoap_pop_item(send_soap);
		}

		else if (!strcmp(string_buff, tr069_parameter_name_table[TR069_PARAM_ManageSerPassword]) ||
			  !strcmp(string_buff, tr069_parameter_name_table[TR069_PARAM_PPPoEPassword]) ||
			  !strcmp(string_buff, tr069_parameter_name_table[TR069_PARAM_UserPassword]) ||
			  !strcmp(string_buff, tr069_parameter_name_table[TR069_PARAM_ConnectionRequestPassword]))
		{
			/* when acs want these passwords, just return empty string */
			param_num++;
			tr069_libsoap_push_item(send_soap, "ParameterValueStruct", NULL, NULL);

			tr069_libsoap_add_item(send_soap, "Name", string_buff, NULL, NULL);
			tr069_libsoap_add_item(send_soap, "Value", "", "xsi:type", "xsd:string");

			tr069_libsoap_pop_item(send_soap);
		}
		else
		{
			ret = tr069_parameter_get_param(param_handle, string_buff, param_result, 0, 0);

			if (-1 == ret)
			{
				tr069_libsoap_free(send_soap);

				fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
				ret = tr069_rpc_build_fault(rpc_handle, fault_code);

				goto FAULT;
			}

			param_num += ret;

			/* get parameters' value and add to soap */
			for (i = 0; i < ret; i++)
			{
				tr069_libsoap_push_item(send_soap, "ParameterValueStruct", NULL, NULL);

				tr069_libsoap_add_item(send_soap, "Name", param_result[i]->param_name, NULL, NULL);
				sprintf(string_buff, "xsd:%s", tr069_data_type_name(param_result[i]->data_type));
				value_buff[0] = 0;
				tr069_parameter_get_value(param_handle, param_result[i], value_buff, TR069_MAX_PARAM_VALUE_LEN);
				tr069_libsoap_add_item(send_soap, "Value", value_buff, "xsi:type", string_buff);

				tr069_libsoap_pop_item(send_soap);
			}
		}

		xml_node = tr069_libsoap_get_next(xml_node);
	}

	tr069_libsoap_pop_item(send_soap);

	sprintf(string_buff, "cwmp:ParameterValueStruct[%d]", param_num);
	tr069_libsoap_add_prop(xml_back, "soap:arrayType", string_buff);

	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, send_soap);

	tr069_libsoap_free(send_soap);

	return 0;

FAULT:

	if (-1 == ret)
	{
		tr069_rpc_clear_fault(rpc_handle);

		return -1;
	}
	else
	{
		tr069_libxml_empty(rpc_handle->xml_buffer);

		tr069_libxml_dump(rpc_handle->xml_buffer, (SoapEnv*)(rpc_handle->fault_soap));

		tr069_rpc_clear_fault(rpc_handle);

		return fault_code;
	}
}

INT32_T tr069_rpc_get_parameter_attributes(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	tr069_parameter *param_result[TR069_MAX_PARAM_NUM];
	INT8_T string_buff[TR069_MAX_PARAM_NAME_LEN] = {0};
	INT8_T value_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	INT8_T *method_name = "cwmp:GetParameterAttributesResponse";
	SoapEnv* send_soap = NULL;
	xmlNodePtr xml_node = NULL;
	xmlNodePtr xml_back = NULL;
	tr069_parameter_mgr *param_handle = NULL;
	INT32_T param_attribute;
	INT32_T param_attribute_1;
	INT32_T param_attribute_2;
	INT32_T param_num;
	INT32_T i;
	INT32_T ret;
	INT32_T fault_code;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	param_handle = rpc_handle->tr069_handle->parameter_handle;
	if (NULL == param_handle)
	{
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	xml_node = tr069_libsoap_get_method(recv_soap);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* ParameterNames */
	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp("ParameterNames", (char *)xml_node->name) != 0)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	xml_node = tr069_libsoap_get_child(xml_node);

	tr069_libsoap_method_new(TR069_URN, method_name, &send_soap);
	if (NULL == send_soap)
	{
		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	xml_back = tr069_libsoap_get_method(send_soap);
	if (NULL == xml_back)
	{
		tr069_libsoap_free(send_soap);

		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	param_num = 0;
	xml_back = tr069_libsoap_push_item(send_soap, "ParameterList", NULL, NULL);

	while (xml_node != NULL)
	{
		if (strcmp("ParameterName", (char *)xml_node->name) != 0
		&& strcmp("string", (char *)xml_node->name) != 0)
		{
			tr069_libsoap_free(send_soap);

			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strlen(tr069_libsoap_get_content(xml_node)) >= TR069_MAX_PARAM_NAME_LEN)
		{
			tr069_libsoap_free(send_soap);

			fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}
		strcpy(string_buff, tr069_libsoap_get_content(xml_node));

		/* get parameters by parameter name */
		if (!strcmp(string_buff, "Device.LAN.DNSServers"))
		{
			/* dnsservers means "dnsserver0, dnsserver1" */
			ret = tr069_parameter_get_param(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer0Real], param_result, 0, 0);
			if (-1 == ret)
			{
				/* fault 9005 invalid parameter name*/
				tr069_libsoap_free(send_soap);
				fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
				ret = tr069_rpc_build_fault(rpc_handle, fault_code);

				goto FAULT;
			}
			ret = tr069_parameter_get_param(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer1Real], param_result + 1, 0, 0);
			if (-1 == ret)
			{
				/* fault 9005 invalid parameter name*/
				tr069_libsoap_free(send_soap);
				fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
				ret = tr069_rpc_build_fault(rpc_handle, fault_code);

				goto FAULT;
			}
			tr069_parameter_get_attribute(param_handle, param_result[0], &param_attribute_1, TR069_MAX_PARAM_VALUE_LEN);
			
			tr069_parameter_get_attribute(param_handle, param_result[1], &param_attribute_2, TR069_MAX_PARAM_VALUE_LEN);

			snprintf(value_buff, sizeof(value_buff), "%d,%d", param_attribute_1, param_attribute_2);

			param_num++;
			tr069_libsoap_push_item(send_soap, "ParameterAttributeStruct", NULL, NULL);

			tr069_libsoap_add_item(send_soap, "Name", string_buff, NULL, NULL);
			tr069_libsoap_add_item(send_soap, "Notification", value_buff, "xsi:type", "xsd:int");

			tr069_libsoap_pop_item(send_soap);
		}
		else
		{
			ret = tr069_parameter_get_param(param_handle, string_buff, param_result, 0, 0);

			if (-1 == ret)
			{
				tr069_libsoap_free(send_soap);

				fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
				ret = tr069_rpc_build_fault(rpc_handle, fault_code);

				goto FAULT;
			}

			param_num += ret;

			/* get parameters' value and add to soap */
			for (i = 0; i < ret; i++)
			{
				tr069_libsoap_push_item(send_soap, "ParameterAttributeStruct", NULL, NULL);

				tr069_libsoap_add_item(send_soap, "Name", param_result[i]->param_name, NULL, NULL);
				sprintf(string_buff, "xsd:%s", tr069_data_type_name(param_result[i]->data_type));

				tr069_parameter_get_attribute(param_handle, param_result[i], &param_attribute, TR069_MAX_PARAM_VALUE_LEN);

				value_buff[0] = 0;
				snprintf(value_buff, sizeof(value_buff), "%d", param_attribute);
				tr069_libsoap_add_item(send_soap, "Notification", value_buff, "xsi:type", "xsd:int");

				tr069_libsoap_pop_item(send_soap);
			}
		}

		xml_node = tr069_libsoap_get_next(xml_node);
	}

	tr069_libsoap_pop_item(send_soap);

	sprintf(string_buff, "cwmp:ParameterAttributeStruct[%d]", param_num);
	tr069_libsoap_add_prop(xml_back, "soap:arrayType", string_buff);

	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, send_soap);

	tr069_libsoap_free(send_soap);

	return 0;

FAULT:

	if (-1 == ret)
	{
		tr069_rpc_clear_fault(rpc_handle);

		return -1;
	}
	else
	{
		tr069_libxml_empty(rpc_handle->xml_buffer);

		tr069_libxml_dump(rpc_handle->xml_buffer, (SoapEnv*)(rpc_handle->fault_soap));

		tr069_rpc_clear_fault(rpc_handle);

		return fault_code;
	}
}


INT32_T tr069_rpc_set_parameter_values(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	INT8_T name_buff[TR069_MAX_PARAM_NAME_LEN] = {0};
	INT8_T value_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	INT8_T *method_name = "cwmp:SetParameterValuesResponse";
	INT8_T *dlmt = NULL;
	SoapEnv* send_soap = NULL;
	xmlNodePtr xml_node = NULL;
	xmlNodePtr xml_set = NULL;
	tr069_parameter_mgr *param_handle = NULL;
	tr069_control_mgr *control_handle = NULL;
	UINT32_T spvfault_number = 0;
	INT32_T ret;
	INT32_T fault_code;
	INT8_T ping_flag = 0;
	INT8_T tracert_flag = 0;
	INT8_T speedtest_flag = 0;
	INT8_T packetlost_flag = 0;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	control_handle = rpc_handle->tr069_handle->control_handle;
	if (NULL == control_handle)
	{
		tr069_error("control handle error\n");
		return -1;
	}

	param_handle = rpc_handle->tr069_handle->parameter_handle;
	if (NULL == param_handle)
	{
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	xml_node = tr069_libsoap_get_method(recv_soap);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* ParameterList */
	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp((char *)xml_node->name, "ParameterList") != 0)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* parameter set fault pre build */
	fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
	ret = tr069_rpc_build_fault(rpc_handle, fault_code);
	if (-1 == ret)
	{
		return -1;
	}

	/* ParameterValueStruct */
	xml_node = tr069_libsoap_get_child(xml_node);
	while (xml_node != NULL)
	{
		if (strcmp((char *)xml_node->name, "ParameterValueStruct") != 0)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		xml_set = tr069_libsoap_get_child(xml_node);
		if (NULL == xml_set)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strcmp((char *)xml_set->name, "Name") != 0)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strlen(tr069_libsoap_get_content(xml_set)) >= TR069_MAX_PARAM_NAME_LEN)
		{
			fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}
		strcpy(name_buff, tr069_libsoap_get_content(xml_set));

		xml_set = tr069_libsoap_get_next(xml_set);
		if (NULL == xml_set)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strcmp((char *)xml_set->name, "Value") != 0)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strlen(tr069_libsoap_get_content(xml_set)) >= TR069_MAX_PARAM_VALUE_LEN)
		{
			fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_VALUE;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}
		strcpy(value_buff, tr069_libsoap_get_content(xml_set));

		if (!strcmp((char *)name_buff, "Device.LAN.DNSServers"))
		{
			ret = 0;
			dlmt = strstr(value_buff, ",");
			if (dlmt != NULL)
			{
				*dlmt = 0;
				dlmt++;
				ret = tr069_parameter_set_value(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer1Real], dlmt, 2);
			}
			if (0 == ret) {
				ret = tr069_parameter_set_value(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer0Real], value_buff, 2);
			}
		}
		else
			ret = tr069_parameter_set_value(param_handle, name_buff, value_buff, 2);

		switch (ret)
		{
		case (0):
			break;

		case (-1):
			tr069_rpc_add_spvf(rpc_handle, name_buff, TR069_FAULT_CPE_INVALID_PARAMETER_NAME);
			spvfault_number++;
			break;

		case (-2):
			tr069_rpc_add_spvf(rpc_handle, name_buff, TR069_FAULT_CPE_INVALID_PARAMETER_VALUE);
			spvfault_number++;
			break;

		case (-3):
			tr069_rpc_add_spvf(rpc_handle, name_buff, TR069_FAULT_CPE_PARAMETER_NOT_WRITABLE);
			spvfault_number++;
			break;

		case (-4):
			/* current set_value can not generate this error */
			tr069_rpc_add_spvf(rpc_handle, name_buff, TR069_FAULT_CPE_INVALID_PARAMETER_TYPE);
			spvfault_number++;
			break;

		case (-5):
			tr069_rpc_add_spvf(rpc_handle, name_buff, TR069_FAULT_CPE_INTERNAL_ERROR);
			spvfault_number++;

		default:
			;
		}

		xml_node = tr069_libsoap_get_next(xml_node);
	}

	if (spvfault_number != 0)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = 0;

		goto FAULT;
	}

	xml_node = tr069_libsoap_get_method(recv_soap);

	xml_node = tr069_libsoap_get_child(xml_node);

	xml_node = tr069_libsoap_get_child(xml_node);

	while (xml_node != NULL)
	{
		xml_set = tr069_libsoap_get_child(xml_node);

		strcpy(name_buff, tr069_libsoap_get_content(xml_set));

		xml_set = tr069_libsoap_get_next(xml_set);

		strcpy(value_buff, tr069_libsoap_get_content(xml_set));

		if (!strcmp((char *)name_buff, "Device.LAN.DNSServers"))
		{
			dlmt = strstr(value_buff, ",");
			if (dlmt != NULL)
			{
				*dlmt = 0;
				dlmt++;
				tr069_parameter_set_value(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer1Real], dlmt, 0);
			}
			tr069_parameter_set_value(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer0Real], value_buff, 0);
		}
		else
			tr069_parameter_set_value(param_handle, name_buff, value_buff, 0);

		if (strcmp(tr069_parameter_name_table[TR069_PARAM_TracertDiagState], name_buff) == 0)
		{
			tracert_flag = 1;
		}
		else if (strcmp(tr069_parameter_name_table[TR069_PARAM_PingDiagState], name_buff) == 0)
		{
			ping_flag = 1;
		}
		else if (strcmp(tr069_parameter_name_table[TR069_PARAM_SpeedTestEnable], name_buff) == 0)
		{
			speedtest_flag = 1;
		}	
		else if (strcmp(tr069_parameter_name_table[TR069_PARAM_PacketLostTestEnable], name_buff) == 0)
		{
			packetlost_flag = 1;
		}	
		xml_node = tr069_libsoap_get_next(xml_node);
	}

	if (1 == tracert_flag)
	{
		/* should insert tracert task to control manager */
		tr069_debug("receive tracert\n");
		tr069_control_put_task(control_handle, TR069_TASK_TRACERT);
	}

	if (1 == ping_flag)
	{
		/* should insert ping task to control manager */
		tr069_debug("receive ping\n");
		tr069_control_put_task(control_handle, TR069_TASK_PING);
	}
	
	if (1 == speedtest_flag && g_speedtest_flag == 0)
	{
		tr069_debug("receive speed test\n");	
		tr069_control_put_task(control_handle, TR069_TASK_SPEED_TEST);
	}

	if (1 == packetlost_flag && g_packetlost_flag == 0)
	{
		tr069_debug("receive packet lost test\n");	
		tr069_packetlost_test();
	}

	tr069_libsoap_method_new(TR069_URN, method_name, &send_soap);
	if (NULL == send_soap)
	{
		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	xml_node = tr069_libsoap_get_method(send_soap);
	if (NULL == xml_node)
	{
		tr069_libsoap_free(send_soap);

		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	tr069_libsoap_add_item(send_soap, "Status", "0", NULL, NULL);

	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, send_soap);

	tr069_libsoap_free(send_soap);

	return 0;

FAULT:

	tr069_debug("fail\n");

	if (-1 == ret)
	{
		tr069_rpc_clear_fault(rpc_handle);

		return -1;
	}
	else
	{
		tr069_libxml_empty(rpc_handle->xml_buffer);

		tr069_libxml_dump(rpc_handle->xml_buffer, (SoapEnv*)(rpc_handle->fault_soap));

		tr069_rpc_clear_fault(rpc_handle);

		return fault_code;
	}
}

INT32_T tr069_rpc_set_parameter_attributes(tr069_rpc_mgr *rpc_handle, SoapEnv *recv_soap)
{
	INT8_T name_buff[TR069_MAX_PARAM_NAME_LEN] = {0};
	INT8_T value_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	INT8_T *method_name = "cwmp:SetParameterAttributesResponse";
	INT8_T *dlmt = NULL;
	SoapEnv* send_soap = NULL;
	xmlNodePtr xml_node = NULL;
	xmlNodePtr xml_set = NULL;
	tr069_parameter_mgr *param_handle = NULL;
	tr069_control_mgr *control_handle = NULL;
	UINT32_T spvfault_number = 0;
	INT32_T ret;
	INT32_T fault_code;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	control_handle = rpc_handle->tr069_handle->control_handle;
	if (NULL == control_handle)
	{
		tr069_error("control handle error\n");
		return -1;
	}

	param_handle = rpc_handle->tr069_handle->parameter_handle;
	if (NULL == param_handle)
	{
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (NULL == recv_soap)
	{
		tr069_error("receive soap error\n");
		return -1;
	}

	xml_node = tr069_libsoap_get_method(recv_soap);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* ParameterList */
	xml_node = tr069_libsoap_get_child(xml_node);
	if (NULL == xml_node)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	if (strcmp((char *)xml_node->name, "ParameterList") != 0)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	/* parameter set fault pre build */
	fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
	ret = tr069_rpc_build_fault(rpc_handle, fault_code);
	if (-1 == ret)
	{
		return -1;
	}

	/* SetParameterAttributesStruct */
	xml_node = tr069_libsoap_get_child(xml_node);
	while (xml_node != NULL)
	{
		if (strcmp((char *)xml_node->name, "SetParameterAttributesStruct") != 0)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		xml_set = tr069_libsoap_get_child(xml_node);
		if (NULL == xml_set)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strcmp((char *)xml_set->name, "Name") != 0)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strlen(tr069_libsoap_get_content(xml_set)) >= TR069_MAX_PARAM_NAME_LEN)
		{
			fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_NAME;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}
		strcpy(name_buff, tr069_libsoap_get_content(xml_set));

		xml_set = tr069_libsoap_get_next(xml_set);
		if (NULL == xml_set)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strcmp((char *)xml_set->name, "NotificationChange") != 0)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strlen(tr069_libsoap_get_content(xml_set)) >= TR069_MAX_PARAM_VALUE_LEN)
		{
			fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_VALUE;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}
		strcpy(value_buff, tr069_libsoap_get_content(xml_set));

		xml_set = tr069_libsoap_get_next(xml_set);
		if (NULL == xml_set)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strcmp((char *)xml_set->name, "Notification") != 0)
		{
			fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}

		if (strlen(tr069_libsoap_get_content(xml_set)) >= TR069_MAX_PARAM_VALUE_LEN)
		{
			fault_code = TR069_FAULT_CPE_INVALID_PARAMETER_VALUE;
			ret = tr069_rpc_build_fault(rpc_handle, fault_code);

			goto FAULT;
		}
		strcpy(value_buff, tr069_libsoap_get_content(xml_set));

		
		if (!strcmp((char *)name_buff, "Device.LAN.DNSServers"))
		{
			ret = 0;
			dlmt = strstr(value_buff, ",");
			if (dlmt != NULL)
			{
				*dlmt = 0;
				dlmt++;
				ret = tr069_parameter_set_attribute(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer1Real], dlmt, 2, 1);
			}
			if (0 == ret) {
				ret = tr069_parameter_set_attribute(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer0Real], value_buff, 2, 1);
			}
		}
		else
			ret = tr069_parameter_set_attribute(param_handle, name_buff, value_buff, 2, 1);

		switch (ret)
		{
		case (0):
			break;

		case (-1):
			tr069_rpc_add_spaf(rpc_handle, name_buff, TR069_FAULT_CPE_INVALID_PARAMETER_NAME);
			spvfault_number++;
			break;

		case (-2):
			tr069_rpc_add_spaf(rpc_handle, name_buff, TR069_FAULT_CPE_INVALID_PARAMETER_VALUE);
			spvfault_number++;
			break;

		case (-3):
			tr069_rpc_add_spaf(rpc_handle, name_buff, TR069_FAULT_CPE_PARAMETER_NOT_WRITABLE);
			spvfault_number++;
			break;

		case (-4):
			/* current set_value can not generate this error */
			tr069_rpc_add_spaf(rpc_handle, name_buff, TR069_FAULT_CPE_INVALID_PARAMETER_TYPE);
			spvfault_number++;
			break;

		case (-5):
			tr069_rpc_add_spaf(rpc_handle, name_buff, TR069_FAULT_CPE_INTERNAL_ERROR);
			spvfault_number++;

		default:
			;
		}

		xml_node = tr069_libsoap_get_next(xml_node);
	}

	if (spvfault_number != 0)
	{
		fault_code = TR069_FAULT_CPE_INVALID_ARGUMENTS;
		ret = 0;

		goto FAULT;
	}

	xml_node = tr069_libsoap_get_method(recv_soap);

	xml_node = tr069_libsoap_get_child(xml_node);

	xml_node = tr069_libsoap_get_child(xml_node);

	while (xml_node != NULL)
	{
		xml_set = tr069_libsoap_get_child(xml_node);

		strcpy(name_buff, tr069_libsoap_get_content(xml_set));

		xml_set = tr069_libsoap_get_next(xml_set);

		strcpy(value_buff, tr069_libsoap_get_content(xml_set));

		if (!strcmp((char *)value_buff, "true"))
		{
			xml_set = tr069_libsoap_get_next(xml_set);

			strcpy(value_buff, tr069_libsoap_get_content(xml_set));

			if (!strcmp((char *)name_buff, "Device.LAN.DNSServers"))
			{
				dlmt = strstr(value_buff, ",");
				if (dlmt != NULL)
				{
					*dlmt = 0;
					dlmt++;
					tr069_parameter_set_attribute(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer1Real], dlmt, 0, 1);
				}
				tr069_parameter_set_attribute(param_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer0Real], value_buff, 0, 1);
			}
			else
				tr069_parameter_set_attribute(param_handle, name_buff, value_buff, 0, 1);
		}
		xml_node = tr069_libsoap_get_next(xml_node);
	}


	tr069_libsoap_method_new(TR069_URN, method_name, &send_soap);
	if (NULL == send_soap)
	{
		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	xml_node = tr069_libsoap_get_method(send_soap);
	if (NULL == xml_node)
	{
		tr069_libsoap_free(send_soap);

		fault_code = TR069_FAULT_CPE_INTERNAL_ERROR;
		ret = tr069_rpc_build_fault(rpc_handle, fault_code);

		goto FAULT;
	}

	tr069_libxml_empty(rpc_handle->xml_buffer);

	tr069_libxml_dump(rpc_handle->xml_buffer, send_soap);

	tr069_libsoap_free(send_soap);

	return 0;

FAULT:

	tr069_debug("fail\n");

	if (-1 == ret)
	{
		tr069_rpc_clear_fault(rpc_handle);

		return -1;
	}
	else
	{
		tr069_libxml_empty(rpc_handle->xml_buffer);

		tr069_libxml_dump(rpc_handle->xml_buffer, (SoapEnv*)(rpc_handle->fault_soap));

		tr069_rpc_clear_fault(rpc_handle);

		return fault_code;
	}
}

INT32_T tr069_rpc_build_fault(tr069_rpc_mgr *rpc_handle, tr069_fault_type fault_code)
{
	INT8_T string_buff[64] = {0};
	xmlNodePtr xml_node = NULL;
	INT8_T *method_name = "soap:Fault";

	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	tr069_rpc_clear_fault(rpc_handle);

	tr069_libsoap_method_new(TR069_URN, method_name, (SoapEnv**)&(rpc_handle->fault_soap));
	if (NULL == rpc_handle->fault_soap)
	{
		return -1;
	}

	xml_node = tr069_libsoap_get_method(rpc_handle->fault_soap);
	if (NULL == xml_node)
	{
		tr069_rpc_clear_fault(rpc_handle);
		return -1;
	}

	tr069_libsoap_add_item(rpc_handle->fault_soap, "faultcode", tr069_fault_type_name(fault_code), NULL, NULL);

	tr069_libsoap_add_item(rpc_handle->fault_soap, "faultstring", "CWMP fault", NULL, NULL);

	tr069_libsoap_push_item(rpc_handle->fault_soap, "detail", NULL, NULL);

	xml_node = tr069_libsoap_push_item(rpc_handle->fault_soap, "cwmp:Fault", NULL, NULL);
	if (NULL == xml_node)
	{
		tr069_rpc_clear_fault(rpc_handle);
		return -1;
	}

	rpc_handle->fault_node = xml_node;

	sprintf(string_buff, "%d", fault_code);
	tr069_libsoap_add_item(rpc_handle->fault_soap, "FaultCode", string_buff, NULL, NULL);

	tr069_libsoap_add_item(rpc_handle->fault_soap, "FaultString", tr069_fault_code_name(fault_code), NULL, NULL);

	tr069_libsoap_pop_item(rpc_handle->fault_soap);

	tr069_libsoap_pop_item(rpc_handle->fault_soap);

	rpc_handle->fault_code = fault_code;

	return 0;
}

INT32_T tr069_rpc_add_spvf(tr069_rpc_mgr *rpc_handle, INT8_T *param_name, tr069_fault_type fault_code)
{
	INT8_T string_buff[64] = {0};

	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	if (NULL == rpc_handle->fault_soap)
	{
		tr069_rpc_build_fault(rpc_handle, TR069_FAULT_CPE_INVALID_ARGUMENTS);
		if (NULL == rpc_handle->fault_soap)
		{
			tr069_error("fault soap struct not exist\n");
			return -1;
		}
	}

	if (NULL == rpc_handle->fault_node)
	{
		tr069_error("fault node not exist\n");
		return -1;
	}

	((SoapEnv*)(rpc_handle->fault_soap))->cur = rpc_handle->fault_node;

	tr069_libsoap_push_item(rpc_handle->fault_soap, "SetParameterValuesFault", NULL, NULL);

	tr069_libsoap_add_item(rpc_handle->fault_soap, "ParameterName", param_name, NULL, NULL);

	sprintf(string_buff, "%d", fault_code);
	tr069_libsoap_add_item(rpc_handle->fault_soap, "FaultCode", string_buff, NULL, NULL);

	tr069_libsoap_add_item(rpc_handle->fault_soap, "FaultString", tr069_fault_code_name(fault_code), NULL, NULL);

	tr069_libsoap_pop_item(rpc_handle->fault_soap);

	return 0;
}

INT32_T tr069_rpc_add_spaf(tr069_rpc_mgr *rpc_handle, INT8_T *param_name, tr069_fault_type fault_code)
{
	INT8_T string_buff[64] = {0};

	if (NULL == rpc_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	if (NULL == rpc_handle->fault_soap)
	{
		tr069_rpc_build_fault(rpc_handle, TR069_FAULT_CPE_INVALID_ARGUMENTS);
		if (NULL == rpc_handle->fault_soap)
		{
			tr069_error("fault soap struct not exist\n");
			return -1;
		}
	}

	if (NULL == rpc_handle->fault_node)
	{
		tr069_error("fault node not exist\n");
		return -1;
	}

	((SoapEnv*)(rpc_handle->fault_soap))->cur = rpc_handle->fault_node;

	tr069_libsoap_push_item(rpc_handle->fault_soap, "SetParameterAttributesFault", NULL, NULL);

	tr069_libsoap_add_item(rpc_handle->fault_soap, "ParameterName", param_name, NULL, NULL);

	sprintf(string_buff, "%d", fault_code);
	tr069_libsoap_add_item(rpc_handle->fault_soap, "FaultCode", string_buff, NULL, NULL);

	tr069_libsoap_add_item(rpc_handle->fault_soap, "FaultString", tr069_fault_code_name(fault_code), NULL, NULL);

	tr069_libsoap_pop_item(rpc_handle->fault_soap);

	return 0;
}

INT8_T* tr069_fault_code_name(tr069_fault_type fault_code)
{
	INT8_T *fault_code_name = NULL;

	switch (fault_code)
	{
	case TR069_FAULT_NO_FAULT:
		fault_code_name = "No Fault";
		break;

	case TR069_FAULT_ACS_METHOD_NOT_SUPPORTED:
		fault_code_name = "Method not supported";
		break;

	case TR069_FAULT_ACS_REQUEST_DENIED:
		fault_code_name = "Request denied (no reason specified)";
		break;

	case TR069_FAULT_ACS_INTERNAL_ERROR:
		fault_code_name = "Internal error";
		break;

	case TR069_FAULT_ACS_INVALID_ARGUMENTS:
		fault_code_name = "Invalid arguments";
		break;

	case TR069_FAULT_ACS_RESOURCES_EXCEEDED:
		fault_code_name = "Resources exceeded";
		break;

	case TR069_FAULT_ACS_RETRY_REQUEST:
		fault_code_name = "Retry request";
		break;

	case TR069_FAULT_CPE_METHOD_NOT_SUPPORTED:
		fault_code_name = "Method not supported";
		break;

	case TR069_FAULT_CPE_REQUEST_DENIED:
		fault_code_name = "Request denied (no reason specified)";
		break;

	case TR069_FAULT_CPE_INTERNAL_ERROR:
		fault_code_name = "Internal error";
		break;

	case TR069_FAULT_CPE_INVALID_ARGUMENTS:
		fault_code_name = "Invalid arguments";
		break;

	case TR069_FAULT_CPE_RESOURCES_EXCEEDED:
		fault_code_name = "Resources exceeded";
		break;

	case TR069_FAULT_CPE_INVALID_PARAMETER_NAME:
		fault_code_name = "Invalid parameter name";
		break;

	case TR069_FAULT_CPE_INVALID_PARAMETER_TYPE:
		fault_code_name = "Invalid parameter type";
		break;

	case TR069_FAULT_CPE_INVALID_PARAMETER_VALUE:
		fault_code_name = "Invalid parameter value";
		break;

	case TR069_FAULT_CPE_PARAMETER_NOT_WRITABLE:
		fault_code_name = "Attempt to set a non-writable parameter";
		break;

	case TR069_FAULT_CPE_NOTIFICATION_REQUEST_REJECTED:
		fault_code_name = "Notification request rejected";
		break;

	case TR069_FAULT_CPE_DOWNLOAD_FAILURE:
		fault_code_name = "Download failure";
		break;

	case TR069_FAULT_CPE_UPLOAD_FAILURE:
		fault_code_name = "Upload failure";
		break;

	default:
		break;
	}

	return fault_code_name;
}

INT8_T* tr069_fault_type_name(tr069_fault_type fault_code)
{
	INT8_T *fault_type_name = NULL;

	switch (fault_code)
	{
	case TR069_FAULT_NO_FAULT:
		fault_type_name = "No Fault";
		break;

	case TR069_FAULT_ACS_METHOD_NOT_SUPPORTED:
	case TR069_FAULT_ACS_REQUEST_DENIED:
	case TR069_FAULT_ACS_INTERNAL_ERROR:
	case TR069_FAULT_CPE_NOTIFICATION_REQUEST_REJECTED:
	case TR069_FAULT_CPE_DOWNLOAD_FAILURE:
	case TR069_FAULT_CPE_UPLOAD_FAILURE:
	case TR069_FAULT_CPE_RESOURCES_EXCEEDED:
	case TR069_FAULT_ACS_RESOURCES_EXCEEDED:
	case TR069_FAULT_ACS_RETRY_REQUEST:
	case TR069_FAULT_CPE_METHOD_NOT_SUPPORTED:
	case TR069_FAULT_CPE_REQUEST_DENIED:
	case TR069_FAULT_CPE_INTERNAL_ERROR:
		fault_type_name = "Server";
		break;

	case TR069_FAULT_ACS_INVALID_ARGUMENTS:
	case TR069_FAULT_CPE_INVALID_ARGUMENTS:
	case TR069_FAULT_CPE_INVALID_PARAMETER_NAME:
	case TR069_FAULT_CPE_INVALID_PARAMETER_TYPE:
	case TR069_FAULT_CPE_INVALID_PARAMETER_VALUE:
	case TR069_FAULT_CPE_PARAMETER_NOT_WRITABLE:
		fault_type_name = "Client";
		break;

	default:
		break;
	}

	return fault_type_name;
}

/*****************************************************************************************
Function:
	receive callback for libcurl to deal with data received

Input:
	buffer: received data from libcurl
	size: fixed to 1
	nmemb: data byte count received
	userp: user pointer set by CURLOPT_WRITEDATA, we use it as tr069_rpc_recv_buff

Output:

Return:
	data byte count received
*****************************************************************************************/
size_t tr069_rpc_recv_callback(INT8_T *buffer, size_t size, size_t nmemb, INT8_T *userp)
{
	tr069_rpc_recv_buff *recv_buff = (tr069_rpc_recv_buff*)userp;
	UINT32_T recv_bytes = size * nmemb;

	if (recv_bytes != 0)
	{
		memcpy(recv_buff->data + recv_buff->number, buffer, recv_bytes);
		recv_buff->number += recv_bytes;
	}

	return recv_bytes;
}

/*****************************************************************************************
Function:
	load management server parameters form config and set to libcurl

Input:
	rpc_handle: handle to rpc module involved

Output:

Return:
	-1: set fail
	0: set success
*****************************************************************************************/
INT32_T tr069_rpc_set_libcurl(tr069_rpc_mgr *rpc_handle)
{
	tr069_interface_mgr *interface_handle = NULL;
	tr069_parameter_mgr *param_handle = NULL;
	tr069_parameter *temp_param = NULL;
	VOID_T *curl_handle = NULL;
	INT8_T *buffer;
	INT8_T user_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	INT8_T pass_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	INT8_T string_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	int i = 0;

	if (NULL == rpc_handle || NULL == rpc_handle->tr069_handle)
	{
		tr069_error("rpc handle error\n");
		return -1;
	}

	interface_handle = rpc_handle->tr069_handle->interface_handle;
	if (NULL == interface_handle || NULL == interface_handle->curl_handle)
	{
		tr069_error("interface handle error\n");
		return -1;
	}

	curl_handle = interface_handle->curl_handle;
	if (NULL == curl_handle)
	{
		tr069_error("curl handle error\n");
		return -1;
	}

	param_handle = rpc_handle->tr069_handle->parameter_handle;
	if (NULL == param_handle)
	{
		tr069_error("parameter handle error\n");
		return -1;
	}

	/* set receive buffer */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, (curl_write_callback)tr069_rpc_recv_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &(rpc_handle->recv_buff));

	/* set acs url */
	tr069_parameter_get_param(param_handle, tr069_parameter_name_table[TR069_PARAM_ManageSerURL], &temp_param, 0, 0);
	tr069_parameter_get_value(param_handle, temp_param, string_buff, TR069_MAX_PARAM_VALUE_LEN);
	curl_easy_setopt(curl_handle, CURLOPT_URL, string_buff);
	/* set send buffer */
	buffer = (INT8_T*)tr069_libxml_get_content(rpc_handle->xml_buffer);
	if (NULL == buffer)
	{
		tr069_error("xml buffer error\n");
		return -1;
	}

	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, buffer);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)strlen(buffer));

	/* set content type */
	/* fixme: should clear content-type of empty post */

#ifdef TR069_AUTH
	tr069_parameter_get_param(param_handle, tr069_parameter_name_table[TR069_PARAM_ManageSerUsername], &temp_param, 0, 0);
	tr069_parameter_get_value(param_handle, temp_param, user_buff, TR069_MAX_PARAM_VALUE_LEN);
	tr069_parameter_get_param(param_handle, tr069_parameter_name_table[TR069_PARAM_ManageSerPassword], &temp_param, 0, 0);
	tr069_parameter_get_value(param_handle, temp_param, pass_buff, TR069_MAX_PARAM_VALUE_LEN);
	sprintf(string_buff,"%s:%s", user_buff, pass_buff);
	curl_easy_setopt(curl_handle, CURLOPT_USERPWD, string_buff);
#endif

	curl_easy_setopt(interface_handle->curl_handle,
		CURLOPT_HTTPHEADER, interface_handle->slist);

	return 0;
}
