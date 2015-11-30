/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: RPC
	Function: RPC support for tr069
********************************************************************/
#ifndef _TR069_RPC_
#define _TR069_RPC_

#include <n_stb_tr069.h>
#include "tr069_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TR069_RPC_RECV_BUFF_SIZE (64 * 1024)

/* use this struct to fit libcurl's recv callback */
typedef struct _tr069_rpc_recv_buff
{
	/* bytes count received */
	UINT32_T number;
	/* received data */
	INT8_T data[TR069_RPC_RECV_BUFF_SIZE];
}tr069_rpc_recv_buff;

/* rpc manager representing rpc sub model */
typedef struct _tr069_rpc_mgr
{
	/* handle to tr069 manager */
	tr069_mgr *tr069_handle;

	/* last request sent to acs */
	tr069_rpc_type req_to_acs;
	/* last request received from acs(RPC) */
	tr069_rpc_type req_from_acs;
	/* xml buffer containing soap message */
	VOID_T *xml_buffer;
	/* current fault soap struct */
	VOID_T *fault_soap;
	/* current fault occured */
	tr069_fault_type fault_code;
	/* point to fault soap's "Fault" node into where we insert "SetParameterValuesFault" node */
	VOID_T *fault_node;
	/* buffer to store http response from acs */
	tr069_rpc_recv_buff recv_buff;
}tr069_rpc_mgr;

tr069_rpc_mgr* tr069_rpc_init(tr069_mgr *tr069_handle);

tr069_rpc_mgr* tr069_rpc_uninit(tr069_rpc_mgr *rpc_handle);

VOID_T tr069_rpc_clear(tr069_rpc_mgr *rpc_handle);

INT32_T tr069_rpc_send(tr069_rpc_mgr *rpc_handle);

INT32_T tr069_rpc_deal_recv(tr069_rpc_mgr *rpc_handle, INT8_T *recv_data);

VOID_T tr069_rpc_clear_fault(tr069_rpc_mgr *rpc_handle);

INT32_T tr069_rpc_build_fault(tr069_rpc_mgr *rpc_handle, tr069_fault_type fault_code);

INT32_T tr069_rpc_add_spvf(tr069_rpc_mgr *rpc_handle, INT8_T *param_name, tr069_fault_type fault_code);

INT32_T tr069_rpc_add_spaf(tr069_rpc_mgr *rpc_handle, INT8_T *param_name, tr069_fault_type fault_code);

INT32_T tr069_rpc_build_inform(tr069_rpc_mgr *rpc_handle, VOID_T *event, VOID_T *inform_parameters, UINT32_T retry_count);

INT32_T tr069_rpc_build_empty(tr069_rpc_mgr *rpc_handle);

INT32_T tr069_rpc_build_transfer_complete(tr069_rpc_mgr *rpc_handle, INT8_T *command_key, INT8_T *start_time, INT8_T *complete_time, tr069_fault_type result);

size_t tr069_rpc_recv_callback(char *buffer, size_t size, size_t nmemb, char *userp);

INT32_T tr069_rpc_set_libcurl(tr069_rpc_mgr *rpc_handle);

INT8_T* tr069_file_type_name(tr069_file_type file_type);

INT8_T* tr069_fault_type_name(tr069_fault_type fault_code);

INT8_T* tr069_event_type_name(tr069_event_type event_type);

INT8_T* tr069_fault_code_name(tr069_fault_type fault_code);

INT8_T* tr069_rpc_type_name(tr069_rpc_type rpc_type);

#ifdef __cplusplus
}
#endif

#endif
