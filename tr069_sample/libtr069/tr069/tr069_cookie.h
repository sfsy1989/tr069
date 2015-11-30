#ifndef __TR069_COOKIE_H__
#define __TR069_COOKIE_H__

#include "tr069_rpc.h"

int tr069_set_req_cookie(tr069_rpc_mgr *rpc_handle);
int tr069_clr_req_cookie(tr069_rpc_mgr *rpc_handle);
int tr069_clear_cookie(tr069_rpc_mgr *rpc_handle);
int tr069_load_cookie(tr069_rpc_mgr *rpc_handle);
int tr069_save_cookie(tr069_rpc_mgr *rpc_handle);

#endif
