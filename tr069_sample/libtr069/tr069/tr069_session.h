/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: Session
	Function: Session Management with ACS
********************************************************************/
#ifndef _TR069_SESSION_
#define _TR069_SESSION_

#include <n_stb_tr069.h>
#include "tr069_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* work status types of session manager */
typedef enum
{
	/* session is not active */
	TR069_SESSION_STATUS_STOP,
	/* send inform request and receive inform response */
	TR069_SESSION_STATUS_INFORM,
	/* send other request and receive other response */
	TR069_SESSION_STATUS_REQUEST,
	/* receive rpc request and send rpc response */
	TR069_SESSION_STATUS_RESPONSE,
	/* if acs send rpc with "hold request", session should execute rpc */
	TR069_SESSION_STATUS_EXECUTE
}tr069_session_status;

/* session manager representing session sub model */
typedef struct _tr069_session_mgr
{
	/* handle to tr069 manager */
	tr069_mgr *tr069_handle;

	/* status */
	tr069_session_status status;
	/* hold request flag */
	INT8_T hold_request;
	/* set parameter flag */
	INT8_T set_param_flag;
}tr069_session_mgr;

tr069_session_mgr* tr069_session_init(tr069_mgr *tr069_handle);

tr069_session_mgr* tr069_session_uninit(tr069_session_mgr *session_handle);

INT32_T tr069_session_clear(tr069_session_mgr *session_handle);

INT32_T tr069_session_main(tr069_session_mgr *session_handle, VOID_T *event, VOID_T *inform_parameters);

#ifdef __cplusplus
}
#endif

#endif

