#ifndef __CFG_SERVICE_H__
#define __CFG_SERVICE_H__

int cfg_service_get_config(int config_cmd, int config_size, char *config_value);
int cfg_service_set_config(int config_cmd, char *config_value);
int cfg_service_get_attribute(int config_cmd, int config_size, int *config_value);
int cfg_service_set_attribute(char *config_cmds);
int cfg_service_sync_config(int config_cmd, char *config_value, int value_len);
int cfg_service_sync_tr069(int config_cmd, char *config_value, int value_len);
int cfg_service_save_config(void);
int cfg_service_save_tr069(void);
int cfg_service_save_iptv(void);
int cfg_service_save_stb_info(void);
int cfg_service_save_product();
int cfg_service_reload_config(void);
int cfg_service_factory_reset(void);
int cfg_service_read_usb_ini(char * usb_ini_file_path);
int cfg_service_copy_ini_file(int which_usb_ini_file);

#endif
