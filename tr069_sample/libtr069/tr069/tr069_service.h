/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: Service
	Function: Service interfaces(internal)
********************************************************************/
#ifndef _TR069_SERVICE_
#define _TR069_SERVICE_

#include <n_stb_tr069.h>
#include "tr069_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* tr069 services, should be called by dispatch */
INT32_T tr069_service_start ();

INT32_T tr069_service_stop ();

INT32_T tr069_service_set_event (int event_index, char event_value, char *command_key);

INT32_T tr069_service_put_request (int request);

INT32_T tr069_service_put_inform_parameter (char *param_name);

INT32_T tr069_service_get_status (char *name, char **value);

INT32_T tr069_service_set_status (char *name, char *value);

INT32_T tr069_service_set_transfer_complete (char *start_time, char *complete_time, int transfer_result);

INT32_T tr069_service_cu_usb_config();

#ifdef __cplusplus
}
#endif

#endif
