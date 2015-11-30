/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: Interface
	Function: Interface package for libs
********************************************************************/
#include <n_lib.h>
#include "tr069_interface.h"
#include "n_stb_cfg.h"
#include "tr069_timer.h"
#include "tr069_statistics.h"
#include "tr069.h"
#include "tr069_service.h"
#include "tr069_session.h"


/* global */

/* extern */
extern int g_tr069_stand;
extern tr069_mgr *g_tr069;
extern int tr069_index_map[TR069_PARAM_NUM];

/* local */
static INT32_T tr069_libsoap_doc_new(xmlDocPtr doc, SoapEnv ** out);
static int tr069_get_fixed_value(int index, char *value, unsigned int size);

/********************************************************
	add node like:
	<[name] [prop_name]="[prop_value]"></[name]>
	move current position to added node
	prop_name and prop_value can be NULL
********************************************************/
xmlNodePtr tr069_libsoap_push_item(SoapEnv *call, INT8_T *name, INT8_T *prop_name, INT8_T *prop_value)
{
	xmlNodePtr newnode;

	if ((newnode = tr069_libsoap_add_item(call, name, "", prop_name, prop_value)))
	{
		call->cur = newnode;
		return newnode;
	}
	else
		return NULL;
}

/********************************************************
	return to parent node
********************************************************/
VOID_T tr069_libsoap_pop_item(SoapEnv * call)
{
	call->cur = call->cur->parent;

	return;
}

/********************************************************
	add node like:
	<[name] [prop_name]="[prop_value]">[value]</[name]>
	prop_name and prop_value can be NULL
********************************************************/
xmlNodePtr tr069_libsoap_add_item(SoapEnv *call, INT8_T *name, INT8_T *value, INT8_T *prop_name, INT8_T *prop_value)
{
	xmlNodePtr newnode;

	newnode = xmlNewTextChild(call->cur, NULL, BAD_CAST name, BAD_CAST value);	
	call->cur->last->ns = NULL;
	if (newnode == NULL)
		return NULL;

	if (prop_name != NULL && prop_value != NULL)
		if (!xmlNewProp(newnode, BAD_CAST prop_name, BAD_CAST prop_value))
			return NULL;

	return newnode;
}

/********************************************************
	add prop to node
********************************************************/
VOID_T tr069_libsoap_add_prop(xmlNodePtr param, INT8_T *prop_name, INT8_T *prop_value)
{
	xmlNewProp(param, BAD_CAST prop_name, BAD_CAST prop_value);

	return;
}

/********************************************************
	create empty soap struct(with given method) like:
	<soap:Envelope xmlns:soap="soap_env_ns"
	xmlns:cwmp="[urn]"
	xmlns:xsi="soap_xsi_ns"
	xmlns:xsd="soap_xsd_ns">
	<soap:Body>
	<[method]>
	</[method]>
	</soap:Body>
	</soap:Envelope>
	urn and method can't be NULL
********************************************************/
INT32_T tr069_libsoap_method_new(INT8_T *urn, INT8_T *method, SoapEnv ** out)
{
	xmlChar buffer[1054];
	xmlChar fixed_method[64];

	if (urn == NULL ||  method == NULL)
		return -1;

	/* adapt to libsoap, add "a:" to save "cwmp:" */
	//sprintf((char *)fixed_method, "a:%s", method);
	sprintf((char *)fixed_method, "%s", method);

	sprintf((char *)buffer, TR069_SOAP_TEMPLATE,
		soap_env_ns, urn,  soap_xsi_ns,
		soap_xsd_ns, (char *)fixed_method, (char *)fixed_method);

	if (tr069_libsoap_buffer_new((INT8_T *)buffer, out) != 0)
		return -1;
	else
		return 0;
}

/********************************************************
	create soap struct with given string in buffer
********************************************************/
INT32_T tr069_libsoap_buffer_new(INT8_T *buffer, SoapEnv ** out)
{
	xmlDocPtr doc;
	INT32_T err;

	if (buffer == NULL)
		return -1;

	if (!(doc = xmlParseDoc(BAD_CAST buffer))){
		tr069_debug("xmlParseDoc error!\n");
		return -1;
	}
	if ((err = tr069_libsoap_doc_new(doc, out)) != 0)
	{
		tr069_debug("tr069_libsoap_doc_new error!\n");
		xmlFreeDoc(doc);
	}
	return err;
}

/********************************************************
	create soap struct with given xmlDoc
	internal only
********************************************************/
static INT32_T tr069_libsoap_doc_new(xmlDocPtr doc, SoapEnv ** out)
{
	xmlNodePtr node;
	SoapEnv *env;

	if (doc == NULL)
	{
		return -1;
	}

	if (!(node = xmlDocGetRootElement(doc)))
	{
		tr069_debug("xmlDocGetRootElement error!\n");
		return -1;
	}

	if (!(env = (SoapEnv *) malloc(sizeof(SoapEnv))))
	{
		return -1;
	}

	env->root = node;
	env->header = tr069_libsoap_get_header(env);
	env->body = tr069_libsoap_get_body(env);
	env->cur = tr069_libsoap_get_method(env);

	*out = env;

	return 0;
}

/********************************************************
	free a soap struct, including all resources within
*********************************************************/
VOID_T tr069_libsoap_free(SoapEnv *env)
{
	if (env)
	{
		if (env->root)
		{
			xmlFreeDoc(env->root->doc);
		}
		free(env);
	}

	return;
}

/********************************************************
	get method node from soap struct
********************************************************/
xmlNodePtr tr069_libsoap_get_method(SoapEnv *env)
{
	xmlNodePtr body;

	if (!(body = tr069_libsoap_get_body(env)))
	{
		return NULL;
	}

	/* method is the first child */
	return tr069_libsoap_get_child(body);
}

/********************************************************
	get child node, skipping non-element node
********************************************************/
xmlNodePtr tr069_libsoap_get_child(xmlNodePtr param)
{
	xmlNodePtr child;

	if (param == NULL)
	{
		return NULL;
	}

	child = param->xmlChildrenNode;
	while (child != NULL)
	{
		if (child->type != XML_ELEMENT_NODE)
			child = child->next;
		else
			break;
	}

	return child;
}

/********************************************************
	get next node, skipping non-element node
********************************************************/
xmlNodePtr tr069_libsoap_get_next(xmlNodePtr param)
{
	xmlNodePtr node = param->next;

	while (node != NULL)
	{
		if (node->type != XML_ELEMENT_NODE)
			node = node->next;
		else
			break;
	}

	return node;
}

/********************************************************
	get body node form soap struct
********************************************************/
xmlNodePtr tr069_libsoap_get_body(SoapEnv * env)
{
	xmlNodePtr node;

	if (env == NULL)
	{
		return NULL;
	}

	if (env->root == NULL)
	{
		return NULL;
	}

	for (node = tr069_libsoap_get_child(env->root); node; node = tr069_libsoap_get_next(node))
	{
		if (!xmlStrcmp(node->name, BAD_CAST "Body")
			&& !xmlStrcmp(node->ns->href, BAD_CAST soap_env_ns))
			return node;
	}

	return NULL;
}

/********************************************************
	get header node from soap struct
********************************************************/
xmlNodePtr tr069_libsoap_get_header(SoapEnv *env)
{
	xmlNodePtr node;

	if (!env)
	{
		return NULL;
	}

	if (!env->root)
	{
		return NULL;
	}

	for (node = tr069_libsoap_get_child(env->root); node; node = tr069_libsoap_get_next(node))
	{
		if (!xmlStrcmp(node->name, BAD_CAST "Header")
			&& !xmlStrcmp(node->ns->href, BAD_CAST soap_env_ns))
			return node;
	}

	return NULL;
}


/********************************************************
	get content string from node
********************************************************/
INT8_T* tr069_libsoap_get_content(xmlNodePtr param)
{
	if (param->children == NULL)
	{
		return "";
	}

	if (param->children->type == XML_TEXT_NODE && param->children->content != NULL)
	{
		return (INT8_T *)param->children->content;
	}
	else
	{
		return NULL;
	}
}

/********************************************************
	get content string from xml buffer
********************************************************/
xmlChar* tr069_libxml_get_content(const xmlBufferPtr buf)
{
	return (xmlChar*)xmlBufferContent(buf);
}

/********************************************************
	empty xml buffer
********************************************************/
VOID_T tr069_libxml_empty(xmlBufferPtr buf)
{
	xmlBufferEmpty(buf);
}

/********************************************************
	dump soap struct to xml buffer
********************************************************/
INT32_T tr069_libxml_dump(xmlBufferPtr buf, SoapEnv *env)
{
	return xmlNodeDump(buf, env->root->doc, env->root, 1, 0);
}

/********************************************************
	create xml buffer
********************************************************/
xmlBufferPtr tr069_libxml_create(VOID_T)
{
	return xmlBufferCreate();
}

/********************************************************
	free xml buffer
********************************************************/
VOID_T tr069_libxml_free(xmlBufferPtr buf)
{
	xmlBufferFree(buf);
}

/********************************************************
	free parser global resources
********************************************************/
VOID_T tr069_libxml_cleanup()
{
	xmlCleanupParser();
}

tr069_interface_mgr* tr069_interface_init(tr069_mgr *tr069_handle)
{
	INT32_T ret;
	tr069_interface_mgr *manager = NULL;

	tr069_debug("enter\n");
	if (NULL == tr069_handle)
	{
		tr069_error("tr069 handle error\n");
		goto ERROR_EXIT;
	}

	/* initialize interface manager */
	manager = malloc(sizeof(tr069_interface_mgr));
	if (NULL == manager)
	{
		tr069_error("malloc interface manager error\n");
		goto ERROR_EXIT;
	}
	memset(manager, 0, sizeof(tr069_interface_mgr));

	/* initialize all elements */
	manager->tr069_handle = tr069_handle;
	manager->curl_handle = NULL;
	manager->slist = NULL;
	manager->curl_global_inited = 0;

	ret = curl_global_init(CURL_GLOBAL_WIN32);
	if (ret != 0)
	{
		tr069_error("initialize libcurl error\n");
		goto ERROR_EXIT;
	}
	manager->curl_global_inited = 1;

	manager->curl_handle = curl_easy_init();
	if (NULL == manager->curl_handle)
	{
		tr069_error("get libcurl handle error\n");
		goto ERROR_EXIT;
	}

	/* set curl to default */
	ret = tr069_libcurl_defaut(manager);
	if (ret != 0)
	{
		tr069_error("set curl default error\n");
		goto ERROR_EXIT;
	}

	tr069_debug("ok quit\n");
	return manager;

ERROR_EXIT:

	/* if interface manager already initialized, uninitialize it */
	if (manager != NULL)
	{
		manager = tr069_interface_uninit(manager);
	}

	tr069_error("error quit\n");
	return NULL;
}

tr069_interface_mgr* tr069_interface_uninit(tr069_interface_mgr *interface_handle)
{
	tr069_debug("enter\n");
	if (interface_handle != NULL)
	{

		if (interface_handle->slist != NULL)
		{
			curl_slist_free_all(interface_handle->slist);
			interface_handle->slist = NULL;
		}

		if (interface_handle->curl_handle != NULL)
		{
			curl_easy_cleanup(interface_handle->curl_handle);
			interface_handle->curl_handle = NULL;
		}

		if (1 == interface_handle->curl_global_inited)
		{
			curl_global_cleanup();
			interface_handle->curl_global_inited = 0;
		}

		tr069_libxml_cleanup();

		free(interface_handle);
	}

	tr069_debug("ok quit\n");
	return NULL;
}

INT32_T tr069_libcurl_defaut(tr069_interface_mgr *interface_handle)
{
	INT32_T ret;

	tr069_debug("enter\n");
	if (NULL == interface_handle)
	{
		tr069_error("interface handle error\n");
		goto ERROR_EXIT;
	}

	/* set default curl timeout */
	ret = curl_easy_setopt(interface_handle->curl_handle, CURLOPT_TIMEOUT, TR069_MAX_CURL_TIMEOUT);
	if (ret != CURLE_OK)
	{
		tr069_error("set curl timeout error\n");
		goto ERROR_EXIT;
	}

	curl_easy_setopt(interface_handle->curl_handle, CURLOPT_VERBOSE, 1);

#ifdef TR069_AUTH
	/* set default curl authentication */
	ret = curl_easy_setopt(interface_handle->curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
	if (ret != CURLE_OK)
	{
		tr069_error("set curl http digest auth error\n");
		goto ERROR_EXIT;
	}
#endif

	interface_handle->slist =
		curl_slist_append(interface_handle->slist,
				  "Content-Type: application/soap+xml");

	tr069_debug("ok quit\n");
	return 0;

ERROR_EXIT:

	tr069_error("error quit\n");
	return -1;
}

static char tr069_diag_ping[TR069_PARAM_PingDiagMaxRespTime - TR069_PARAM_PingDiagState + 1][256];

static char tr069_diag_traceroute[TR069_PARAM_HOPHOST32 - TR069_PARAM_TracertDiagState + 1][256];

static char tr069_diag_speed_test[TR069_PARAM_SpeedTestResult - TR069_PARAM_SpeedTestUniqueNum + 1][256];

static char tr069_diag_packetlost_test[TR069_PARAM_PacketLostTestBand - TR069_PARAM_PacketLostTestEnable + 1][256]; 


void tr069_clear_route_host()
{
	int i;
	for (i = 8; i < 40; i++) {
		tr069_diag_traceroute[i][0] = '\0';
	}

}

int tr069_set_config(int index, char *value)
{
	INT32_T ret;
	INT8_T cur_value[TR069_MAX_PARAM_VALUE_LEN] = {0};

	if (index >= TR069_PARAM_TracertDiagState && index <= TR069_PARAM_HOPHOST32)
	{
	    snprintf(tr069_diag_traceroute[index - TR069_PARAM_TracertDiagState],
	    	     256,
	    	     value);
	    ret = -1;
	}
	else if (index >= TR069_PARAM_PingDiagState && index <= TR069_PARAM_PingDiagMaxRespTime)
	{
	    snprintf(tr069_diag_ping[index - TR069_PARAM_PingDiagState],
		     256,
		     value);
	    ret = -1;
	}
	else if (index == TR069_PARAM_IPAddressReal)
	{
		n_cfg_set_config(F_CFG_IPAddress, value);
		ret = FV_OK;
	}
	else if (index == TR069_PARAM_SubnetMaskReal)
	{
		n_cfg_set_config(F_CFG_SubnetMask, value);
		ret = FV_OK;
	}
	else if (index == TR069_PARAM_DefaultGatewayReal)
	{
		n_cfg_set_config(F_CFG_DefaultGateway, value);
		ret = FV_OK;
	}
	else if (index == TR069_PARAM_DNSServer0Real)
	{
		n_cfg_set_config(F_CFG_DNSServer0, value);
		ret = FV_OK;
	}
	else if (index == TR069_PARAM_DNSServer1Real)
	{
		n_cfg_set_config(F_CFG_DNSServer1, value);
		ret = FV_OK;
	}
	else if (index >= TR069_PARAM_SpeedTestUniqueNum && index <= TR069_PARAM_SpeedTestResult)
	{
		snprintf(tr069_diag_speed_test[index - TR069_PARAM_SpeedTestUniqueNum],
	    	     256,
	    	     value);
	    ret = -1;
	}
	else if (index >= TR069_PARAM_PacketLostTestEnable && index <= TR069_PARAM_PacketLostTestBand)
	{
		snprintf(tr069_diag_packetlost_test[index - TR069_PARAM_PacketLostTestEnable],
	    	     256,
	    	     value);
	    ret = -1;
	}
	else
	{
	    tr069_get_config(index, cur_value, sizeof(cur_value));
	    if (strcmp(cur_value, value) != 0)
	    {
			n_cfg_set_config(tr069_index_map[index], value);
			ret = FV_OK;

			/* check for set config's special effect */
			if (index >= TR069_PARAM_PeriodicInformEnable &&
				index <= TR069_PARAM_PeriodicInformTime)
			{
				tr069_start_heartbeat();
			}
			else if (index == TR069_PARAM_LogMsgEnable ||
				index == TR069_PARAM_LogMsgOrFile ||
				index == TR069_PARAM_LogMsgDuration)
			{
				tr069_start_logmsg();
			}
			else if (index == TR069_PARAM_LogUploadInterval)
			{
				n_iptv_reset_statis_upload();
			}
			else if (index >= TR069_PARAM_SQMEnableEPG &&
			         index <= TR069_PARAM_SQMEnableWarning)
			{
				n_iptv_log_sqm_ctrl(atoi(value));
			}
			else if (index == TR069_PARAM_SQMMediaInterval)
			{
				n_iptv_log_sqm_ctrl(1);
			}
			else if (index == TR069_PARAM_SQMEnableTelnet)
			{
				system("telnetd");
			}
	    }
	    else
	    {
	    	ret = -1;
	    }
	}
	if (ret == FV_OK) {
		((tr069_session_mgr *)(g_tr069->session_handle))->set_param_flag = 1;
	}
	return FV_OK;
}

int tr069_get_config(int index, char *value, unsigned int size)
{
	INT8_T string_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};

	tr069_debug("index=%d\n", index);

	if (index == TR069_PARAM_CurrentLocalTime)
	{
	    return tr069_format_time(time(NULL), value, size);
	}
	else if (index >= TR069_PARAM_ConnectionUpTime && index <= TR069_PARAM_QuarterHourPacketsReceived)
	{
	    return tr069_get_net_stat(index, value, size);
	}
	else if (index >= TR069_PARAM_Startpoint && index<= TR069_PARAM_PlayErrorInfo)
	{
	    return tr069_get_iptv_stat(index, value, size);
	}
	else if (index >= TR069_PARAM_MultiRRT && index <= TR069_PARAM_BufferIncNmb)
	{
	    return tr069_get_iptv_stat(index, value, size);
	}
	else if (index >= TR069_PARAM_TracertDiagState && index <= TR069_PARAM_HOPHOST32)
	{
	    snprintf(value, size, tr069_diag_traceroute[index - TR069_PARAM_TracertDiagState]);
	    return FV_OK;
	}
	else if (index >= TR069_PARAM_PingDiagState && index <= TR069_PARAM_PingDiagMaxRespTime)
	{
	    snprintf(value, size, tr069_diag_ping[index - TR069_PARAM_PingDiagState]);
	    return FV_OK;
	}
	else if (index == TR069_PARAM_AuthServiceInfoPPPOEEnable)
	{
		n_cfg_get_config(tr069_index_map[TR069_PARAM_AddressingType], string_buff, sizeof(string_buff));
		if (strcmp("PPPOE", string_buff) == 0){
			snprintf(value, size, "true");
			return FV_OK;
		} else {
			snprintf(value, size, "false");
			return FV_OK;
		}
	} else if (TR069_PARAM_ManageSerURL == index) {
		n_cfg_get_config(F_CFG_LocalManageServerUrl, value, size);
		if (0 == strcmp(value, "")) {
			n_cfg_get_config(F_CFG_ManageSerURL, value, size);
		}
		return FV_OK;
	} else if (TR069_PARAM_StorageSize == index) {
		int tmp;
		o_soc_cap_query(O_SC_STORAGE_SIZE, &tmp);
		snprintf(value, size, "%d", tmp);
		return FV_OK;
	} else if (TR069_PARAM_PhyMemSize == index) {
		int tmp;
		o_soc_cap_query(O_SC_PHYMEMSIZE, &tmp);
		snprintf(value, size, "%d", tmp);
		return FV_OK;
	} else if (index >= TR069_PARAM_Description && index <= TR069_PARAM_IsSupportIPv6) {
		return tr069_get_fixed_value(index, value, size);
	}
	else if (index >= TR069_PARAM_SpeedTestUniqueNum && index <= TR069_PARAM_SpeedTestResult)
	{
		snprintf(value, size, tr069_diag_speed_test[index - TR069_PARAM_SpeedTestUniqueNum]);
	    return FV_OK;
	}
	else if (index >= TR069_PARAM_PacketLostTestEnable && index <= TR069_PARAM_PacketLostTestBand)
	{
		snprintf(value, size, tr069_diag_packetlost_test[index - TR069_PARAM_PacketLostTestEnable]);
	    return FV_OK;
	}
	else
	{
	    return n_cfg_get_config(tr069_index_map[index], value, size);
	}
}

void tr069_save_all_config()
{
	n_cfg_save_config();
	n_cfg_save_iptv();
	n_cfg_save_tr069();
}

int tr069_offline_notify()
{
	int ret;

	n_cfg_set_config(tr069_index_map[TR069_PARAM_DeviceStatus], "OFFLINE");
	ret = tr069_service_set_event(TR069_EVENT_VALUE_CHANGE, 1, "");
	if (ret == FV_OK) {
		tr069_service_put_inform_parameter(PCODE_DeviceStatus);
		tr069_service_put_inform_parameter(PCODE_IPAddressReal);
		tr069_service_start();
	}

	return FV_OK;

}

/* used for value change notification only */
static int tr069_format_param_code(int config_name, char *param_code, int size)
{
	switch (config_name) {
	case F_CFG_Manufacturer:
		snprintf(param_code, size - 1, "%s", PCODE_Manufacturer);
		break;
	case F_CFG_ManufacturerOUI:
		snprintf(param_code, size - 1, "%s", PCODE_ManufacturerOUI);
		break;
	case F_CFG_ProductClass:
		snprintf(param_code, size - 1, "%s", PCODE_ProductClass);
		break;
	case F_CFG_SerialNumber:
		snprintf(param_code, size - 1, "%s", PCODE_SerialNumber);
		break;
	case F_CFG_HardwareVersion:
		snprintf(param_code, size - 1, "%s", PCODE_HardwareVersion);
		break;
	case F_CFG_FsVersion:
		snprintf(param_code, size - 1, "%s", PCODE_SoftwareVersion);
		break;
	case F_CFG_ProvisioningCode:
		snprintf(param_code, size - 1, "%s", PCODE_ProvisioningCode);
		break;
	case F_CFG_DeviceStatus:
		snprintf(param_code, size - 1, "%s", PCODE_DeviceStatus);
		break;
	case F_CFG_KernelVersion:
		snprintf(param_code, size - 1, "%s", PCODE_KernelVersion);
		break;
	case F_CFG_StreamingControlProtocols:
		snprintf(param_code, size - 1, "%s", PCODE_StrmCtlProtocols);
		break;
	case F_CFG_StreamingTransportProtocols:
		snprintf(param_code, size - 1, "%s", PCODE_StrmTrsptProtocols);
		break;
	case F_CFG_StreamingTransportControlProtocols:
		snprintf(param_code, size - 1, "%s", PCODE_StrmTrsptCtlProtocols);
		break;
	case F_CFG_DownloadTransportProtocols:
		snprintf(param_code, size - 1, "%s", PCODE_DldTrsptProtocols);
		break;
	case F_CFG_MultiplexTypes:
		snprintf(param_code, size - 1, "%s", PCODE_MultiplexTypes);
		break;
	case F_CFG_MaxDejitteringBufferSize:
		snprintf(param_code, size - 1, "%s", PCODE_MaxDjtBufferSize);
		break;
	case F_CFG_AudioStandards:
		snprintf(param_code, size - 1, "%s", PCODE_AudioStandards);
		break;
	case F_CFG_VideoStandards:
		snprintf(param_code, size - 1, "%s", PCODE_VideoStandards);
		break;
	case F_CFG_ManageSerURL:
		snprintf(param_code, size - 1, "%s", PCODE_ManageSerURL);
		break;
	case F_CFG_ManageSerUsername:
		snprintf(param_code, size - 1, "%s", PCODE_ManageSerUsername);
		break;
	case F_CFG_ManageSerPassword:
		snprintf(param_code, size - 1, "%s", PCODE_ManageSerPassword);
		break;
	case F_CFG_ConnectionRequestURL:
		snprintf(param_code, size - 1, "%s", PCODE_ConnectionRequestURL);
		break;
	case F_CFG_ConnectionRequestUsername:
		snprintf(param_code, size - 1, "%s", PCODE_ConnectionRequestUsername);
		break;
	case F_CFG_ConnectionRequestPassword:
		snprintf(param_code, size - 1, "%s", PCODE_ConnectionRequestPassword);
		break;
	case F_CFG_PeriodicInformEnable:
		snprintf(param_code, size - 1, "%s", PCODE_PeriodicInformEnable);
		break;
	case F_CFG_PeriodicInformInterval:
		snprintf(param_code, size - 1, "%s", PCODE_PeriodicInformInterval);
		break;
	case F_CFG_PeriodicInformTime:
		snprintf(param_code, size - 1, "%s", PCODE_PeriodicInformTime);
		break;
	case F_CFG_UpgradesManaged:
		snprintf(param_code, size - 1, "%s", PCODE_UpgradesManaged);
		break;
	case F_CFG_ParameterKey:
		snprintf(param_code, size - 1, "%s", PCODE_ParameterKey);
		break;
	case F_CFG_NTPServer1:
		snprintf(param_code, size - 1, "%s", PCODE_NTPServer1);
		break;
	case F_CFG_LocalTimeZone:
		snprintf(param_code, size - 1, "%s", PCODE_LocalTimeZone);
		break;
	case F_CFG_CurrentLocalTime:
		snprintf(param_code, size - 1, "%s", PCODE_CurrentLocalTime);
		break;
	case F_CFG_Device:
		snprintf(param_code, size - 1, "%s", PCODE_Device);
		break;
	case F_CFG_DhcpUser:
		snprintf(param_code, size - 1, "%s", PCODE_DhcpUser);
		break;
	case F_CFG_DhcpPassword:
		snprintf(param_code, size - 1, "%s", PCODE_DhcpPassword);
		break;
	case F_CFG_AddressingType:
		snprintf(param_code, size - 1, "%s", PCODE_AddressingType);
		break;
	case F_CFG_IPAddressReal:
		snprintf(param_code, size - 1, "%s", PCODE_IPAddressReal);
		break;
	case F_CFG_SubnetMaskReal:
		snprintf(param_code, size - 1, "%s", PCODE_SubnetMaskReal);
		break;
	case F_CFG_DefaultGatewayReal:
		snprintf(param_code, size - 1, "%s", PCODE_DefaultGatewayReal);
		break;
	case F_CFG_DNSServer0Real:
		snprintf(param_code, size - 1, "%s", PCODE_DNSServer0Real);
		break;
	case F_CFG_DNSServer1Real:
		snprintf(param_code, size - 1, "%s", PCODE_DNSServer1Real);
		break;
	case F_CFG_MACAddress:
		snprintf(param_code, size - 1, "%s", PCODE_MACAddress);
		break;
	case F_CFG_STBID:
		snprintf(param_code, size - 1, "%s", PCODE_STBID);
		break;
	case F_CFG_PPPoEID:
		snprintf(param_code, size - 1, "%s", PCODE_PPPoEID);
		break;
	case F_CFG_PPPoEPassword:
		snprintf(param_code, size - 1, "%s", PCODE_PPPoEPassword);
		break;
	case F_CFG_UserID:
		snprintf(param_code, size - 1, "%s", PCODE_UserID);
		break;
	case F_CFG_UserPassword:
		snprintf(param_code, size - 1, "%s", PCODE_UserPassword);
		break;
	case F_CFG_LogServerUrl:
		snprintf(param_code, size - 1, "%s", PCODE_LogServerUrl);
		break;
	case F_CFG_LogUploadInterval:
		snprintf(param_code, size - 1, "%s", PCODE_LogUploadInterval);
		break;
	case F_CFG_LogRecordInterval:
		snprintf(param_code, size - 1, "%s", PCODE_LogRecordInterval);
		break;
	case F_CFG_StatInterval:
		snprintf(param_code, size - 1, "%s", PCODE_StatInterval);
		break;
	case F_CFG_OperatorInfo:
		snprintf(param_code, size - 1, "%s", PCODE_OperatorInfo);
		break;
	case F_CFG_UpgradeDomain:
		snprintf(param_code, size - 1, "%s", PCODE_UpgradeURL);
		break;
	case F_CFG_AuthURL:
		snprintf(param_code, size - 1, "%s", PCODE_AuthURL);
		break;
	case F_CFG_AuthURLBackup:
		snprintf(param_code, size - 1, "%s", PCODE_BrowserURL2);
		break;
	case F_CFG_AdministratorPassword:
		snprintf(param_code, size - 1, "%s", PCODE_AdministratorPassword);
		break;
	case F_CFG_CUserPassword:
		snprintf(param_code, size - 1, "%s", PCODE_CUserPassword);
		break;
	case F_CFG_UserProvince:
		snprintf(param_code, size - 1, "%s", PCODE_UserProvince);
		break;
	case F_CFG_UDPConnectionRequestAddress:
		snprintf(param_code, size - 1, "%s", PCODE_UDPConnectionRequestAddress);
		break;
	case F_CFG_UDPConnectionRequestNtfLimit:
		snprintf(param_code, size - 1, "%s", PCODE_UDPConnectionRequestNtfLimit);
		break;
	case F_CFG_STUNEnable:
		snprintf(param_code, size - 1, "%s", PCODE_STUNEnable);
		break;
	case F_CFG_STUNServerAddress:
		snprintf(param_code, size - 1, "%s", PCODE_STUNServerAddress);
		break;
	case F_CFG_STUNServerPort:
		snprintf(param_code, size - 1, "%s", PCODE_STUNServerPort);
		break;
	case F_CFG_STUNUsername:
		snprintf(param_code, size - 1, "%s", PCODE_STUNUsername);
		break;
	case F_CFG_STUNPassword:
		snprintf(param_code, size - 1, "%s", PCODE_STUNPassword);
		break;
	case F_CFG_STUNMaxKeepAlivePeriod:
		snprintf(param_code, size - 1, "%s", PCODE_STUNMaxKeepAlivePeriod);
		break;
	case F_CFG_STUNMinKeepAlivePeriod:
		snprintf(param_code, size - 1, "%s", PCODE_STUNMinKeepAlivePeriod);
		break;
	default:
		tr069_error("param code no match\n");
		return -1;
	}

	return FV_OK;
}

static void tr069_value_change(char *key)
{
	int ret = FV_TRUE;
	int i;
	int not_null = 0;
	int stb_flag = 1;
	char key_tr069[256];

	for (i = 0; i < F_CFG_NUM; i++) {
		if (key[i] == '1') {
			if (FV_OK == tr069_format_param_code(i, key_tr069, sizeof(key_tr069))) {
				tr069_service_put_inform_parameter(key_tr069);
				not_null = 1;
			}
		}
		if (i == F_CFG_STBID && key[i] == '1') {
			stb_flag = 0;
		}
	}
	if (not_null == 1) {
		ret = tr069_service_set_event(TR069_EVENT_VALUE_CHANGE, 1, "");
		if (stb_flag == 1) {
			tr069_service_put_inform_parameter(PCODE_STBID);
		}
	}

	if (ret == FV_OK) {
		tr069_service_put_inform_parameter(PCODE_IPAddressReal);
		tr069_service_start();
	}
	
	return;
}

int tr069_value_change_notify(char *param)
{
	if (g_tr069_stand == TR069_STAND_CU) {
		tr069_value_change(param);
	}
	
	return FV_OK;
}

int tr069_set_parameter_attribute(int index, char *value)
{
	char config_cmds[F_CFG_NUM + 1];
	int i;

	for (i = 0; i < F_CFG_NUM; i++) {
		config_cmds[i] = '2';
	}

	config_cmds[F_CFG_NUM] = '\0';
	
	if (*value == '2') {
		//*value = '1';
		config_cmds[tr069_index_map[index]] = '1';
	} else {
		config_cmds[tr069_index_map[index]] = *value;
	}

	n_cfg_set_parameter_attribute(config_cmds);

	return FV_OK;
}

int tr069_alarm_report()
{
	int ret;

	ret = tr069_service_set_event(TR069_EVENT_X_CU_ALARM, 1, "");
	if (ret == FV_OK) {
		tr069_service_put_inform_parameter(PCODE_AlarmReason);
		tr069_service_put_inform_parameter(PCODE_AlarmPacketsLostRate);
		tr069_service_put_inform_parameter(PCODE_AlarmFramesLost);
		tr069_service_put_inform_parameter(PCODE_STBID);
		tr069_service_put_inform_parameter(PCODE_IPAddressReal);
		tr069_service_start();
	}

	return FV_OK;
}

fboolean is_double(char const* s)
{
    int n;
    double d;
    return sscanf(s, "%lf %n", &d, &n) == 1 && !s[n];
}

fboolean is_int(char const* s)
{
	int n;
	int i;
	return sscanf(s, "%d %n", &i, &n) == 1 && !s[n];
}

static int tr069_get_fixed_value(int index, char *value, unsigned int size)
{
	if (TR069_PARAM_IsSupportIPv6 == index) {
#if FTM_ENABLE(IPV6)
		snprintf(value, size, "true");
#else 
		snprintf(value, size, "false");
#endif
		return FV_OK;
	} else if (TR069_PARAM_CurrentLanguage == index) {
		snprintf(value, size, "zh");
		return FV_OK;
	} else if (TR069_PARAM_AvailableLanguages == index) {
		snprintf(value, size, "zh+en");
		return FV_OK;
	} else if (TR069_PARAM_UpTime == index) {
		snprintf(value, size, "1800");
		return FV_OK;
	} else if (TR069_PARAM_FirstUseDate == index) {
		snprintf(value, size, "2013-04-16,09:30");
		return FV_OK;
	} else if (TR069_PARAM_DeviceLog == index || TR069_PARAM_ConfigFile == index) {
		return FV_OK;
	} else if (TR069_PARAM_Description == index) {
		snprintf(value, size, "FONSVIEW_STB");
		return FV_OK;
	} else if (TR069_PARAM_ConfigFileVersion == index) {
		snprintf(value, size, "v1.3");
		return FV_OK;
	} else {
		return -1;
	}
}
