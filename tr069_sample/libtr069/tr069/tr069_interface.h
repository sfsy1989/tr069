/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: Interface
	Function: Interface package for libs
********************************************************************/
#ifndef _TR069_INTERFACE_
#define _TR069_INTERFACE_

#include "soap-env.h"
#include "libxml/tree.h"
#include "curl/curl.h"

#include <n_stb_tr069.h>
#include "tr069_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* max curl performance timeout in sec */
#define TR069_MAX_CURL_TIMEOUT 30

/* soap template created by interface */
#define TR069_SOAP_TEMPLATE \
	"<soap:Envelope xmlns:soap=\"%s\"" \
	" xmlns:cwmp=\"%s\"" \
	" xmlns:xsi=\"%s\"" \
	" xmlns:xsd=\"%s\">" \
	"<soap:Body>"\
	"<%s>"\
	"</%s>" \
	"</soap:Body>"\
	"</soap:Envelope>"

/* interface manager packing several interfaces */
typedef struct _tr069_interface_mgr
{
	tr069_mgr *tr069_handle;
	VOID_T *curl_handle;
	INT8_T curl_global_inited;
	struct curl_slist *slist;
}tr069_interface_mgr;

xmlNodePtr tr069_libsoap_push_item(SoapEnv *call, INT8_T *name, INT8_T *propname, INT8_T *propvalue);

VOID_T tr069_libsoap_pop_item(SoapEnv * call);

xmlNodePtr tr069_libsoap_add_item(SoapEnv *call, INT8_T *name, INT8_T *value, INT8_T *propname, INT8_T *propvalue);

VOID_T tr069_libsoap_add_prop(xmlNodePtr param, INT8_T *propname, INT8_T *propvalue);

INT32_T tr069_libsoap_method_new(INT8_T *urn, INT8_T *method, SoapEnv ** out);

INT32_T tr069_libsoap_buffer_new(INT8_T *buffer, SoapEnv ** out);

VOID_T tr069_libsoap_free(SoapEnv *env);

xmlNodePtr tr069_libsoap_get_method(SoapEnv *env);

xmlNodePtr tr069_libsoap_get_child(xmlNodePtr param);

xmlNodePtr tr069_libsoap_get_next(xmlNodePtr param);

xmlNodePtr tr069_libsoap_get_body(SoapEnv * env);

xmlNodePtr tr069_libsoap_get_header(SoapEnv *env);

INT8_T* tr069_libsoap_get_content(xmlNodePtr param);

xmlChar* tr069_libxml_get_content(const xmlBufferPtr buf);

VOID_T tr069_libxml_empty(xmlBufferPtr buf);

INT32_T tr069_libxml_dump(xmlBufferPtr buf, SoapEnv *env);

xmlBufferPtr tr069_libxml_create(VOID_T);

VOID_T tr069_libxml_free(xmlBufferPtr buf);

VOID_T tr069_libxml_cleanup();

tr069_interface_mgr* tr069_interface_init(tr069_mgr *tr069_handle);

tr069_interface_mgr* tr069_interface_uninit(tr069_interface_mgr *interface_handle);

INT32_T tr069_libcurl_defaut(tr069_interface_mgr *interface_handle);

int tr069_set_config(int index, char *value);

int tr069_get_config(int index, char *value, unsigned int size);

void tr069_save_all_config();

int tr069_offline_notify();

int tr069_value_change_notify(char *param);

int tr069_set_parameter_attribute(int index, char *value);

int tr069_alarm_report();

fboolean is_double(char const* s);

fboolean is_int(char const* s);

#ifdef __cplusplus
}
#endif

#endif
