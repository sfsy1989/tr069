#ifndef __CFG_H__
#define __CFG_H__

#include "n_lib.h"

typedef struct {
	int ini_file;
	char *cmd;
	char *default_value;
	int bufsize;
	char tr069_notify;
	int modify_flag;
} cfg_params_info_s;

int cfg_get_ini_bufsize(int id);
char *cfg_get_ini_cmd(int id);
int cfg_get_param_int(int id);
void cfg_set_param_int(int id, int val);
#ifdef __cplusplus
extern "C" {
#endif
int config_task();
#ifdef __cplusplus
}
#endif
int cfg_init();
int cfg_uninit();
double cfg_get_param_double(int id);
void cfg_set_param_double(int id, double val);
char *cfg_get_param_string(int id);
int cfg_get_param_string_len(int id);
void cfg_set_param_string(int id, char *val);
int cfg_set_dynamic_param_string(int id, char *val);
int cfg_set_param_attribute(int id, char attribute);
int cfg_get_param_attribute(int id);
int cfg_set_param_flag(int id, int value);
int cfg_params_load();
int cfg_params_save();
int cfg_dynamic_params_save();
int cfg_tr069_save();
int cfg_iptv_save();
int cfg_stb_info_save();
int cfg_product_save();
int cfg_params_sync(int sync_cmd, char *sync_value);
int cfg_tr069_sync(int sync_cmd, char *sync_value);
void cfg_file_is_exist();

enum {
	CONFIG_INI_FILE = 0,
	CONFIG_DYNAMIC_INI_FILE,
	PRODUCT_INI_FILE,
	STB_INI_FILE,
	IPTV_INI_FILE,
	TR069_INI_FILE,
	INI_FILE_TOTAL,
	INI_FILE_NONE
};
#endif

