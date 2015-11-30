#include <stdlib.h>
#include <stdio.h>

#include "cfg.h"
#include "cfg_service.h"
#include "cfg_dispatch.h"
#include "cfg_util.h"

#include "n_lib.h"

static int
cfg_ipc_get_config(ipc_valueset * arg, ipc_valueset * ret)
{
	int config_cmd = F_CFG_NUM;
	int config_size;
	char config_value[N_CFG_MAX_VALUE_SIZE];

	ipc_deform_valueset (arg, "ii", &config_cmd, &config_size);
	cfg_service_get_config(config_cmd, config_size, config_value);
	ipc_form_valueset (ret, "s", config_value);

	return FV_OK;
}

static int
cfg_ipc_set_config(ipc_valueset * arg, ipc_valueset * ret)
{
	int config_cmd;
	char config_value[N_CFG_MAX_VALUE_SIZE] = { 0 };

	ipc_deform_valueset(arg, "is", &config_cmd, config_value);
	cfg_service_set_config(config_cmd, config_value);
	ipc_form_valueset(ret, "i", FV_OK);

	return FV_OK;
}

static int
cfg_ipc_set_config_attribute(ipc_valueset * arg, ipc_valueset * ret)
{
	char config_cmds[F_CFG_NUM + 1];

	ipc_deform_valueset(arg, "s", config_cmds);
	cfg_service_set_attribute(config_cmds);

	return FV_OK;
}

static int
cfg_ipc_sync_config(ipc_valueset * arg, ipc_valueset * ret)
{
	int config_cmd;
	int value_len;
	char config_value[F_IPC_MAX_VALUESET_SIZE] = { 0 };

	ipc_deform_valueset (arg, "iis", &config_cmd, &value_len, config_value);
	cfg_service_sync_config (config_cmd, config_value, value_len);
	ipc_form_valueset (ret, "i", FV_OK);

	return FV_OK;
}

static int
cfg_ipc_sync_tr069(ipc_valueset * arg, ipc_valueset * ret)
{
	int config_cmd;
	int value_len;
	char config_value[F_IPC_MAX_VALUESET_SIZE] = { 0 };

	ipc_deform_valueset (arg, "iis", &config_cmd, &value_len, config_value);
	cfg_service_sync_tr069 (config_cmd, config_value, value_len);
	ipc_form_valueset (ret, "i", FV_OK);

	return FV_OK;
}

static int
cfg_ipc_reload_config(ipc_valueset * arg, ipc_valueset * ret)
{
	cfg_service_reload_config ();
	ipc_form_valueset (ret, "i", FV_OK);
	return FV_OK;
}

static int
cfg_ipc_save_config(ipc_valueset * arg, ipc_valueset * ret)
{
	cfg_service_save_config();
	ipc_form_valueset(ret, "i", FV_OK);
	return FV_OK;
}

static int
cfg_ipc_save_tr069(ipc_valueset * arg, ipc_valueset * ret)
{
	cfg_service_save_tr069();
	ipc_form_valueset(ret, "i", FV_OK);
	return FV_OK;
}

static int
cfg_ipc_save_iptv(ipc_valueset * arg, ipc_valueset * ret)
{
	cfg_service_save_iptv();
	ipc_form_valueset(ret, "i", FV_OK);
	return FV_OK;
}

static int
cfg_ipc_save_stb_info(ipc_valueset * arg, ipc_valueset * ret)
{
	cfg_service_save_stb_info();
	ipc_form_valueset(ret, "i", FV_OK);
	return FV_OK;
}

static int
cfg_ipc_factory_reset(ipc_valueset * arg, ipc_valueset * ret)
{
	cfg_service_factory_reset ();
	ipc_form_valueset (ret, "i", FV_OK);
	return FV_OK;
}

static int cfg_ipc_copy_ini_file(ipc_valueset * arg, ipc_valueset * ret)
{
	int which_usb_ini_file;
	
	ipc_deform_valueset (arg, "i", &which_usb_ini_file);
	cfg_service_copy_ini_file(which_usb_ini_file);
	return FV_OK;
}

static int cfg_ipc_usb_ini_read(ipc_valueset * arg, ipc_valueset * ret)
{
	char usb_ini_file_path[128] = {0};
	
	ipc_deform_valueset (arg, "s", usb_ini_file_path);
	cfg_service_read_usb_ini(usb_ini_file_path);
	return FV_OK;
}

static int cfg_ipc_set_burnmac_timer(ipc_valueset * arg, ipc_valueset * ret)
{
	cfg_burn_mac_action action = CFG_BURN_MAC_NONE;
	ipc_deform_valueset (arg, "i", &action);
	if(CFG_BURN_MAC_START == action) {
		//fixme
		//cfg_burn_mac_start();
		ipc_form_valueset (ret, "i", FV_OK);
	} else if (CFG_BURN_MAC_STOP == action) {
		//fixme
		//cfg_burn_mac_stop();
		ipc_form_valueset (ret, "i", FV_OK);
	} else {
		ipc_form_valueset (ret, "i", -1);
	}

	return FV_OK;
}

int cfg_dispatch_init()
{
	ipc_service *service = NULL;

	service = n_init_service(N_MOD_CFG, NULL, 0);
	f_assert(NULL != service);
	if (NULL == service) {
		return -1;
	}

	ipc_add_interface (service, cmd_cfg_get_config, cfg_ipc_get_config);
	ipc_add_interface (service, cmd_cfg_set_config, cfg_ipc_set_config);
	ipc_add_interface (service, cmd_cfg_set_attribute, cfg_ipc_set_config_attribute);
	ipc_add_interface (service, cmd_cfg_sync_config, cfg_ipc_sync_config);
	ipc_add_interface (service, cmd_cfg_sync_tr069, cfg_ipc_sync_tr069);
	ipc_add_interface (service, cmd_cfg_reload_config, cfg_ipc_reload_config);
	ipc_add_interface (service, cmd_cfg_save_config, cfg_ipc_save_config);
	ipc_add_interface (service, cmd_cfg_save_tr069, cfg_ipc_save_tr069);
	ipc_add_interface (service, cmd_cfg_save_iptv, cfg_ipc_save_iptv);
	ipc_add_interface (service, cmd_cfg_save_stb_info, cfg_ipc_save_stb_info);
	ipc_add_interface (service, cmd_cfg_factory_reset, cfg_ipc_factory_reset);
	ipc_add_interface (service, cmd_cfg_usb_ini_read, cfg_ipc_usb_ini_read);
	ipc_add_interface (service, cmd_cfg_set_burnmac_timmer, cfg_ipc_set_burnmac_timer);
	ipc_add_interface (service, cmd_cfg_copy_ini_file, cfg_ipc_copy_ini_file);

	return FV_OK;
}

int
cfg_dispatch_uninit()
{
	return FV_OK;
}

int
cfg_dispatch_start()
{
	n_fvipc_start(F_IPC_TIMER_NORMAL);
	return FV_OK;
}

int
cfg_dispatch_stop()
{
	n_fvipc_stop();
	return FV_OK;
}

