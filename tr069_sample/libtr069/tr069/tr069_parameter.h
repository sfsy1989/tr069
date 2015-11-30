/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: Parameter
	Function: Parameter management and maintenance
********************************************************************/
#ifndef _TR069_PARAMETER_
#define _TR069_PARAMETER_

#include <n_stb_tr069.h>
#include "tr069_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* max parameter id number in stack */
#define TR069_PARAM_IDSTACK_SIZE 16

/* parameter node which form the parameter tree */
typedef struct _tr069_parameter
{
	/* parameter identifier */
	UINT16_T param_id;
	/* write enable flag */
	UINT8_T write_flag;
	/* number of child node, non-zero means object */
	UINT8_T child_number;
	/* parameter value length allocated */
//	UINT16_T value_len;
	/* parameter name */
	INT8_T *param_name;
	/* parameter index */
	INT32_T param_index;
 	/* parameter value */
// 	INT8_T *param_value;
	/* parameter data type */
	tr069_data_type data_type;
	/* parameter notification type */
	tr069_notification_type notif_type;
	/* next sibling node */
	struct _tr069_parameter *next;
	/* first child node */
	struct _tr069_parameter *child;
}tr069_parameter;

/* parameter manager representing parameter sub model */
typedef struct _tr069_parameter_mgr
{
	/* handle to tr069 manager */
	tr069_mgr *tr069_handle;

	/* parameter tree pointer, reference to root node "Device." */
	tr069_parameter param_root;
	/* parameter allocating table, map from id to parameter */
	tr069_parameter *param_table[TR069_MAX_PARAM_NUM];
	/* inform parameters table */
//	tr069_parameter *inform_param[TR069_MAX_INFORM_PARAM_NUM];
	/* current parameter number */
	UINT16_T param_number;
	/* allocatable parameter id stack */
	UINT16_T idstack[TR069_PARAM_IDSTACK_SIZE];
	/* stack top, number of id in stack */
	UINT8_T idstack_top;
}tr069_parameter_mgr;

tr069_parameter_mgr* tr069_parameter_init(tr069_mgr *tr069_handle);

tr069_parameter_mgr* tr069_parameter_uninit(tr069_parameter_mgr *param_handle);

tr069_parameter* tr069_parameter_add_param(tr069_parameter_mgr *param_handle, INT8_T *param_name, INT32_T param_config_name, tr069_data_type param_type, tr069_notification_type notif_type, UINT8_T write_flag);

INT32_T tr069_parameter_remove_param(tr069_parameter_mgr *param_handle, INT8_T *param_name);

tr069_parameter* tr069_parameter_find_node(tr069_parameter_mgr *param_handle, INT8_T *param_name, tr069_parameter ***ref_pointer);

INT32_T tr069_parameter_set_value(tr069_parameter_mgr *param_handle, INT8_T *param_name, INT8_T *param_value, UINT8_T force_flag);

INT32_T tr069_parameter_get_value(tr069_parameter_mgr *param_handle, tr069_parameter *param, INT8_T *value_recver, INT32_T value_size);

INT32_T tr069_parameter_get_param(tr069_parameter_mgr *param_handle, INT8_T *param_name, tr069_parameter **param_set, UINT8_T next_level, UINT8_T object_flag);

const INT8_T* tr069_data_type_name(tr069_data_type data_type);



INT32_T tr069_parameter_get_attribute(tr069_parameter_mgr *param_handle, tr069_parameter *param, INT32_T *value_recver, INT32_T value_size);
INT32_T tr069_parameter_set_attribute(tr069_parameter_mgr *param_handle, INT8_T *param_name, INT8_T *param_value, UINT8_T force_flag, UINT8_T mode_flag);

#if 0
INT32_T tr069_parameter_put_inform_param(tr069_parameter_mgr *param_handle, tr069_parameter *param);

VOID_T tr069_parameter_init_inform_parameter(tr069_parameter_mgr *param_handle);

VOID_T tr069_parameter_clear_inform_param(tr069_parameter_mgr *param_handle);
#endif

#ifdef __cplusplus
}
#endif

#endif
