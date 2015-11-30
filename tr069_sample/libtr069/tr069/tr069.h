#ifndef __TR069_H__
#define __TR069_H__

//#include <syslog.h>

#include "n_lib.h"

#define tr069_debug(...)    f_debug(N_LOG_CPE,  __VA_ARGS__)
#define tr069_error(...)    f_error(N_LOG_CPE,  __VA_ARGS__)
#define tr069_notice(...)   f_notice(N_LOG_CPE, __VA_ARGS__)
#define tr069_warning(...)   f_warning(N_LOG_CPE, __VA_ARGS__)
#ifdef __cplusplus
extern "C" {
#endif
int tr069_task();
#ifdef __cplusplus
}
#endif
int tr069_main();
int tr069_load_environment(tr069_mgr *tr069_handle);
int tr069_dump_environment(tr069_mgr *tr069_handle);

tr069_mgr* tr069_init();
tr069_mgr* tr069_uninit(tr069_mgr *tr069_handle);

#endif
