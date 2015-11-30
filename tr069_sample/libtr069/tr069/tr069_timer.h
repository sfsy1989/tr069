#ifndef __TR069_TIMER_H__
#define __TR069_TIMER_H__

int tr069_start_heartbeat();
int tr069_start_logmsg();
int tr069_format_time(time_t now_time, char *tr069_time, int size);
int tr069_unformat_time(char *tr069_time);

int tr069_packetlost_test();

#endif

