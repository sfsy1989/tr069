#ifndef __CPE_DUMP_H__
#define __CPE_DUMP_H__
/**
 \brief cpe dump fields

 declare cpe dump fields
 */
typedef struct {
	/* basic */
    int ver;
    char build_time[128];

	/* advanced */
	int call_flag;
	int config_sem_value;
	int session_sem_value;
	int tr069_status;
} cpe_dump_s;





#endif

