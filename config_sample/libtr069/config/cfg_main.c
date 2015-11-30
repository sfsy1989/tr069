#include <stdio.h>
#include <stdlib.h>
#include "cfg.h"
#include "cfg_dispatch.h"
#include "cfg_util.h"
//fixme
//#include "cfg_log_config.h"
int cfg_reload(void);

static n_lib_callback_s g_cfg_flib_callback = {
	.reload_cfg_func = cfg_reload,
};


int cfg_reload(void)
{
    cfg_params_load();

    return FV_OK;
}

void cfg_recv_int(int sig)
{
    cfg_debug("---- receive exit signal %d\n", sig);
    switch (sig) {
    case SIGINT:
        cfg_dispatch_stop();
        break;
    default:
        break;
    }
}

int config_task()
{
    if(FV_OK != n_lib_init ("config", &g_cfg_flib_callback)){
        goto cfg_init_err_exit;
    }
    //f_daemon_init ();

    if (signal(SIGINT, cfg_recv_int) == SIG_ERR) {
        cfg_error("register signal SIGINT error\n");
        goto cfg_init_err_exit;
    }
    cfg_init();
	cfg_file_is_exist();
    cfg_params_load();
    cfg_dispatch_init ();
    //f_daemon_start ();
    //fixme
    //cfg_log_start();
    n_stb_notify_module_status(N_MOD_RUNNING);
	
    cfg_dispatch_start ();
cfg_init_err_exit:
    cfg_error ("cfg service stop!\n");
    //fixme
    //cfg_log_stop();
    cfg_uninit();
    n_lib_uninit();
    return 0;
}
