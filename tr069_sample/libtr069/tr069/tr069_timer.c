#include "n_lib.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#include "curl/curl.h"
#include "tr069_service.h"
#include "tr069_timer.h"
#include "tr069.h"
#include "tr069_interface.h"

unsigned int tr069_heartbeat_timer = 0;
unsigned int tr069_logmsg_timer = 0;
unsigned int tr069_packetlost_timer = 0;

static int inform_interlval_i;
static int logmsg_interval_i = 10;
static time_t inform_time_t;
static int logmsg_duration_i = 0;

extern int g_packetlost_flag;

static int tr069_logmsg()
{
	int ret;
	char logmsg_enable_s[F_TR069_ENABLE_SIZE];
	n_iptv_sqm_media_msg *sqm_media_ms = 0;
	char buf[50] = {0};
	tr069_debug("logmsg inform triggered %d\n", logmsg_duration_i);
	n_cfg_get_config(F_CFG_LogMsgEnable, logmsg_enable_s, 
		sizeof(logmsg_enable_s));
	if (0 != strncasecmp(logmsg_enable_s, "true", sizeof("true"))) {
		tr069_logmsg_timer = 0;
		return -1;
	}

	if (logmsg_duration_i < 0) {
		/* reset logmsg enable param */
		n_cfg_set_config(F_CFG_LogMsgEnable, "false");
		tr069_logmsg_timer = 0;
		return -1;
	}
	logmsg_duration_i -= 10;
	sqm_media_ms =(n_iptv_sqm_media_msg *) f_get_dump_addr(DP_MPLAYER_SQM,
						sizeof(n_iptv_sqm_media_msg));	
	if(NULL == sqm_media_ms ) {
		tr069_error("dump DP_MPLAYER_SQM failed.\n");
		return -1;
	}
	f_assert(sqm_media_ms);
	ret = tr069_service_set_event(TR069_EVENT_SCHEDULED, 1, "");
	ret = tr069_service_set_event(TR069_EVENT_M_CTC_LOG_PERIODIC, 1, "");
	if (0 == ret) {
		tr069_service_put_inform_parameter(PCODE_STBID);
		tr069_service_put_inform_parameter(PCODE_IPAddressReal);
		tr069_set_config(TR069_PARAM_LogMsgRTSPInfo, "DESCRIBE RTSP 1.0");
		tr069_service_put_inform_parameter(PCODE_LogMsgRTSPInfo);
		tr069_set_config(TR069_PARAM_LogMsgHTTPInfo, "GET HTTP 1.0");
		tr069_service_put_inform_parameter(PCODE_LogMsgHTTPInfo);
		tr069_set_config(TR069_PARAM_LogMsgIGMPInfo, "JOINCHAN");
		tr069_service_put_inform_parameter(PCODE_LogMsgIGMPInfo);
		sprintf(buf, "%llu", sqm_media_ms->mr/188);
		tr069_set_config(TR069_PARAM_LogMsgPkgTotalOneSec, buf);
		tr069_service_put_inform_parameter(PCODE_LogMsgPkgTotalOneSec);
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%llu", sqm_media_ms->mr);
		tr069_set_config(TR069_PARAM_LogMsgByteTotalOneSec, buf);
		tr069_service_put_inform_parameter(PCODE_LogMsgByteTotalOneSec);
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%f", sqm_media_ms->mlr);
		tr069_set_config(TR069_PARAM_LogMsgPkgLostRate, buf);
		tr069_service_put_inform_parameter(PCODE_LogMsgPkgLostRate);
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%llu", sqm_media_ms->mr);
		tr069_set_config(TR069_PARAM_LogMsgAvarageRate, buf);
		tr069_service_put_inform_parameter(PCODE_LogMsgAvarageRate);
		memset(buf, 0, sizeof(buf));
		tr069_set_config(TR069_PARAM_LogMsgBUFFER, "2097152");
		tr069_service_put_inform_parameter(PCODE_LogMsgBUFFER);
		tr069_set_config(TR069_PARAM_LogMsgERROR, "NULL");
		tr069_service_put_inform_parameter(PCODE_LogMsgERROR);
		tr069_set_config(TR069_PARAM_LogMsgVendorExt, "NULL");
		tr069_service_put_inform_parameter(PCODE_LogMsgVendorExt);
		tr069_service_start();
	}

	return FV_OK;
}

static int tr069_heartbeat(void *arg)
{
	int ret;
	char inform_enable_s[F_TR069_ENABLE_SIZE];
	fboolean is_first = (fboolean)arg;

	tr069_debug("periodic inform triggered %d\n", is_first);
	n_cfg_get_config(F_CFG_PeriodicInformEnable, inform_enable_s,
		sizeof(inform_enable_s));
	if (0 != strncasecmp(inform_enable_s, "true", sizeof("true"))) {
		tr069_heartbeat_timer = 0;
		return -1;
	}
	ret = tr069_service_set_event(TR069_EVENT_PERIODIC, 1, "");
	if (0 == ret) {
		tr069_service_put_inform_parameter(PCODE_STBID);
		tr069_service_put_inform_parameter(PCODE_IPAddressReal);
		tr069_service_put_inform_parameter(PCODE_UserID);
		tr069_service_put_inform_parameter(PCODE_HardwareVersion);
		tr069_service_put_inform_parameter(PCODE_SoftwareVersion);
		tr069_service_put_inform_parameter(PCODE_ConnectionRequestURL);
		tr069_service_put_inform_parameter(PCODE_ProvisioningCode);
		tr069_service_put_inform_parameter(PCODE_ConnectionUpTime);
		tr069_service_put_inform_parameter(PCODE_TotalBytesSent);
		tr069_service_put_inform_parameter(PCODE_TotalBytesReceived);
		tr069_service_put_inform_parameter(PCODE_TotalPacketsSent);
		tr069_service_put_inform_parameter(PCODE_TotalPacketsReceived);
		tr069_service_put_inform_parameter(PCODE_CurrentDayInterval);
		tr069_service_put_inform_parameter(PCODE_CurrentDayBytesSent);
		tr069_service_put_inform_parameter(PCODE_CurrentDayBytesReceived);
		tr069_service_put_inform_parameter(PCODE_CurrentDayPacketsSent);
		tr069_service_put_inform_parameter(PCODE_CurrentDayPacketsReceived);
		tr069_service_put_inform_parameter(PCODE_QuarterHourInterval);
		tr069_service_put_inform_parameter(PCODE_QuarterHourBytesSent);
		tr069_service_put_inform_parameter(PCODE_QuarterHourBytesReceived);
		tr069_service_put_inform_parameter(PCODE_QuarterHourPacketsSent);
		tr069_service_put_inform_parameter(PCODE_QuarterHourPacketsReceived);
		tr069_service_put_inform_parameter(PCODE_MACAddress);
		tr069_service_start();
	}

	if (is_first) {
		char inform_interval_s[F_MAX_INTERVAL_SIZE];
		int ret_i = n_cfg_get_config(F_CFG_PeriodicInformInterval,
				inform_interval_s,
				sizeof(inform_interval_s));
		if (0 == ret_i){
			inform_interlval_i = atoi(inform_interval_s);
		}
		else {
			inform_interlval_i = TR069_HEARTBEAT_LIMIT;
		}
		if (inform_interlval_i < TR069_HEARTBEAT_LIMIT) {
			inform_interlval_i = TR069_HEARTBEAT_LIMIT;
		}
		tr069_debug("tr069 inform_interval_i is %d\n", inform_interlval_i);
		tr069_heartbeat_timer = f_timer_add(inform_interlval_i * 1000,
			(f_timer_func)tr069_heartbeat, (void*)FV_FALSE);
		return -1;
	}

	return FV_OK;
}

int tr069_logmsg_timer_stop()
{
	if (0 != tr069_logmsg_timer) {
		f_timer_remove(tr069_logmsg_timer);
		tr069_logmsg_timer = 0;
	}

	return FV_TRUE;
}

int tr069_heartbeat_timer_stop()
{
	if (0 != tr069_heartbeat_timer) {
		f_timer_remove(tr069_heartbeat_timer);
		tr069_heartbeat_timer = 0;
	}

	return FV_TRUE;
}

int tr069_unformat_time(char *tr069_time)
{
	struct tm temp_tm;
	char temp_string[32];

	if (strlen(tr069_time) < 19) {
		return -1;
	}

	memset(&temp_tm, 0, sizeof(struct tm));
	memcpy(temp_string, tr069_time, 4);
	temp_string[4] = 0;
	temp_tm.tm_year = atoi(temp_string) - 1900;
	memcpy(temp_string, tr069_time + 5, 2);
	temp_string[2] = 0;
	temp_tm.tm_mon = atoi(temp_string) - 1;
	memcpy(temp_string, tr069_time + 8, 2);
	temp_string[2] = 0;
	temp_tm.tm_mday = atoi(temp_string);
	memcpy(temp_string, tr069_time + 11, 2);
	temp_string[2] = 0;
	temp_tm.tm_hour = atoi(temp_string);
	memcpy(temp_string, tr069_time + 14, 2);
	temp_string[2] = 0;
	temp_tm.tm_min = atoi(temp_string);
	memcpy(temp_string, tr069_time + 17, 2);
	temp_string[2] = 0;
	temp_tm.tm_sec = atoi(temp_string);

	return mktime(&temp_tm);
}

static int get_1st_heartbeat_time(time_t now_time, time_t interval, time_t ref_time)
{
	time_t seconds_left = 0;

	seconds_left = now_time - ref_time;
	if (seconds_left > 0)
		seconds_left = interval - (seconds_left % interval);
	else
		seconds_left = (-seconds_left) % interval;

	if (seconds_left < 1)
		seconds_left = 1;

	return seconds_left;
}

int tr069_format_time(time_t now_time, char *tr069_time, int size)
{
	struct tm *now_tm;
	/* format : 0001-01-01T00:00:00 */
	if (NULL == tr069_time)
	{
		return -1;
	}

	if (size < 20)
	{
		return -1;
	}

	tzset();
	now_tm = localtime(&now_time);
	sprintf(tr069_time, "%04d-%02d-%02dT%02d:%02d:%02d",
		now_tm->tm_year + 1900,
		now_tm->tm_mon + 1,
		now_tm->tm_mday,
		now_tm->tm_hour,
		now_tm->tm_min,
		now_tm->tm_sec);

	return 0;
}

int tr069_start_logmsg()
{
	char logmsg_enable_s[F_TR069_ENABLE_SIZE];
	char logmsg_ismsg_s[F_TR069_ENABLE_SIZE];
	char logmsg_duration_s[F_MAX_INTERVAL_SIZE];     
	n_cfg_get_config(F_CFG_LogMsgEnable,
		logmsg_enable_s,
		sizeof(logmsg_enable_s));

	n_cfg_get_config(F_CFG_LogMsgOrFile,
		logmsg_ismsg_s,
		sizeof(logmsg_ismsg_s));
	if (!strncasecmp(logmsg_ismsg_s, "true", sizeof("true"))) {
		/* for protocol logmsg */
		n_iptv_reset_logmsg_upload(0);
		if (!strncasecmp(logmsg_enable_s, "true", sizeof("true"))) {
			int ret_i = n_cfg_get_config(F_CFG_LogMsgDuration,
				logmsg_duration_s,
				sizeof(logmsg_duration_s));
			if (0 == ret_i) {
				logmsg_duration_i = atoi(logmsg_duration_s);
			}
			else {
				logmsg_duration_i = TR069_LOGMSG_LIMIT;
			}
			if (logmsg_duration_i > TR069_LOGMSG_LIMIT) {
				logmsg_duration_i = TR069_LOGMSG_LIMIT;
			}
				
			tr069_logmsg_timer_stop();
			tr069_logmsg_timer = f_timer_add(logmsg_interval_i * 1000,
				(f_timer_func)tr069_logmsg, NULL);

			return FV_OK;
			
		}
		else {
			tr069_logmsg_timer_stop();
			return -1;
		}
	}
	else {
		/* for file logmsg */
		tr069_logmsg_timer_stop();
		n_iptv_reset_logmsg_upload(1);
		return FV_OK;
	}
}

int tr069_start_heartbeat()
{
	int first_heartbeat_time;

	char inform_enable_s[F_TR069_ENABLE_SIZE];
	char inform_interval_s[F_MAX_INTERVAL_SIZE];
	char inform_time_s[F_IPTV_TIMESTR_SIZE];

	time_t now_time = time(NULL);

	n_cfg_get_config(F_CFG_PeriodicInformEnable,
			 inform_enable_s,
			 sizeof(inform_enable_s));

	if (!strncasecmp(inform_enable_s, "true", sizeof("true"))) {
		int ret_i = n_cfg_get_config(F_CFG_PeriodicInformInterval,
				     inform_interval_s,
				     sizeof(inform_interval_s));
		
		if (0 == ret_i) {
			inform_interlval_i = atoi(inform_interval_s);
		}
		else {
			inform_interlval_i = TR069_HEARTBEAT_LIMIT;
		}
		if (inform_interlval_i < TR069_HEARTBEAT_LIMIT) {
			inform_interlval_i = TR069_HEARTBEAT_LIMIT;
		}

		ret_i = n_cfg_get_config(F_CFG_PeriodicInformTime, inform_time_s,
						 sizeof(inform_time_s));
		if (0 == ret_i) {
			inform_time_t = tr069_unformat_time(inform_time_s);
		}
		else {
			inform_time_t = 0;
		}

		first_heartbeat_time =
			get_1st_heartbeat_time(now_time, inform_interlval_i, inform_time_t);

		tr069_heartbeat_timer_stop();
		tr069_heartbeat_timer = f_timer_add(first_heartbeat_time * 1000,
			(f_timer_func)tr069_heartbeat, (void*)FV_TRUE);

		return FV_OK;
	}
	else {
		tr069_heartbeat_timer_stop();
		return -1;
	}
}

static int check_packetlost(void *arg)
{
	char string_buff[256] = {0};
	tr069_get_config(TR069_PARAM_PacketLostTestEnable, string_buff, sizeof(string_buff));
	if (!strncasecmp(string_buff, "false", sizeof("false"))) {
		tr069_packetlost_timer = 0;
		g_packetlost_flag = 0;
		n_check_waitms();
		system("/ipstb/bin/fakeir portal");
		system("killall -9 iperf");
		return -1;
	}
	return 0;
}

int tr069_packetlost_test()
{
	char string_buff[256] = {0};
	int port;
	tr069_get_config(TR069_PARAM_PacketLostTestEnable, string_buff, sizeof(string_buff));
	if (!strncasecmp(string_buff, "true", sizeof("true"))) {
		g_packetlost_flag = 1;
		tr069_get_config(TR069_PARAM_PacketLostTestUDPPort, string_buff, sizeof(string_buff));
		port = atoi(string_buff);
		tr069_packetlost_timer = f_timer_add(1000, check_packetlost, NULL);
		if (tr069_packetlost_timer <= 0) {
			tr069_error("f_timer_add error\n");
			return -1;
		}
		n_check_waitms();
		system("/ipstb/bin/go http://127.0.0.1/stb_config/packetlost_test.html");
		sprintf(string_buff, "/ipstb/bin/iperf -s -u -p %d &", port);
		system(string_buff);
		return 0;
	}
	return -1;
}

