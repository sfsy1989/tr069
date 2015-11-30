#include <string.h>
#include <stdio.h>

#include "n_lib.h"
#include "tr069.h"

/* extern */
extern lan_statistics_s *net_stat;
extern brows_iptv_stat_s *brows_stat;
extern mplayer_iptv_statistics_s *mplayer_stat;


int tr069_get_net_stat(int key, char *value, unsigned int size)
{
	/* add by ash: get net statistics */
	*value = 0;

	if (net_stat == NULL) {
		tr069_error("net stat unavailable\n");
		return -1;
	}

	if (TR069_PARAM_ConnectionUpTime == key) {
		snprintf(value, size, "%u", net_stat->ConnectionUpTime);
	} else if (TR069_PARAM_TotalBytesSent == key) {
		snprintf(value, size, "%u", net_stat->TotalBytesSent);
	} else if (TR069_PARAM_TotalBytesReceived == key) {
		snprintf(value, size, "%u", net_stat->TotalBytesReceived);
	} else if (TR069_PARAM_TotalPacketsSent == key) {
		snprintf(value, size, "%u", net_stat->TotalPacketsSent);
	} else if (TR069_PARAM_TotalPacketsReceived == key) {
		snprintf(value, size, "%u", net_stat->TotalPacketsReceived);
	} else if (TR069_PARAM_CurrentDayInterval == key) {
		snprintf(value, size, "%u", net_stat->CurrentDayInterval);
	} else if (TR069_PARAM_CurrentDayBytesSent == key) {
		snprintf(value, size, "%u", net_stat->CurrentDayBytesSent);
	} else if (TR069_PARAM_CurrentDayBytesReceived == key) {
		snprintf(value, size, "%u", net_stat->CurrentDayBytesReceived);
	} else if (TR069_PARAM_CurrentDayPacketsSent == key) {
		snprintf(value, size, "%u", net_stat->CurrentDayPacketsSent);
	} else if (TR069_PARAM_CurrentDayPacketsReceived == key) {
		snprintf(value, size, "%u",
			 net_stat->CurrentDayPacketsReceived);
	} else if (TR069_PARAM_QuarterHourInterval == key) {
		snprintf(value, size, "%u", net_stat->QuarterHourInterval);
	} else if (TR069_PARAM_QuarterHourBytesSent == key) {
		snprintf(value, size, "%u", net_stat->QuarterHourBytesSent);
	} else if (TR069_PARAM_QuarterHourBytesReceived == key) {
		snprintf(value, size, "%u", net_stat->QuarterHourBytesReceived);
	} else if (TR069_PARAM_QuarterHourPacketsSent == key) {
		snprintf(value, size, "%u", net_stat->QuarterHourPacketsSent);
	} else if (TR069_PARAM_QuarterHourPacketsReceived == key) {
		snprintf(value, size, "%u",
			 net_stat->QuarterHourPacketsReceived);
	} else {
		return -1;
	}
	
	return FV_OK;
}

int tr069_get_iptv_stat(int key, char *value, unsigned int size)
{
	*value = 0;
	
	if (brows_stat == NULL) {
		tr069_error("brows stat unavailable\n");
		return -1;
	}
	
	if (mplayer_stat == NULL) {
		tr069_error("mplayer stat unavailable\n");
		return -1;
	}

	if (TR069_PARAM_Startpoint == key) {
		snprintf(value, size, "%s", "");
	} else if (TR069_PARAM_Endpoint ==  key) {
		snprintf(value, size, "%s", "");
	} else if (TR069_PARAM_AuthNumbers ==  key) {
		snprintf(value, size, "%d", brows_stat->AuthNumbers);
	} else if (TR069_PARAM_AuthFailNumbers == key) {
		snprintf(value, size, "%d", brows_stat->AuthFailNumbers);
	} else if (key == TR069_PARAM_AuthFailInfo) {
		strncpy(value, brows_stat->AuthFailInfo[0], size - 1);
	} else if (key == TR069_PARAM_MultiReqNumbers) {
		snprintf(value, size, "%d", mplayer_stat->MultiReqNumbers);
	} else if (key == TR069_PARAM_MultiFailNumbers) {
		snprintf(value, size, "%d", mplayer_stat->MultiFailNumbers);
	} else if (key == TR069_PARAM_MultiFailInfo) {
		strncpy(value, mplayer_stat->MultiFailInfo[0], size - 1);
	} else if (key == TR069_PARAM_VodReqNumbers) {
		snprintf(value, size, "%d", mplayer_stat->VodReqNumbers);
	} else if (key == TR069_PARAM_VodFailNumbers) {
		snprintf(value, size, "%d", mplayer_stat->VodFailNumbers);
	} else if (key == TR069_PARAM_VodFailInfo) {
		strncpy(value, mplayer_stat->VodFailInfo[0], size - 1);
	} else if (key == TR069_PARAM_HTTPReqNumbers) {
		snprintf(value, size, "%d", brows_stat->HTTPReqNumbers);
	} else if (key == TR069_PARAM_HTTPFailNumbers) {
		snprintf(value, size, "%d", brows_stat->HTTPFailNumbers);
	} else if (key == TR069_PARAM_HTTPFailInfo) {
		strncpy(value, brows_stat->HTTPFailInfo[0], size - 1);
	} else if (key == TR069_PARAM_MutiAbendNumbers) {
		snprintf(value, size, "%d", mplayer_stat->MutiAbendNumbers);
	} else if (key == TR069_PARAM_VODAbendNumbers) {
		snprintf(value, size, "%d", mplayer_stat->VODAbendNumbers);
	} else if (key == TR069_PARAM_PlayErrorNumbers) {
		snprintf(value, size, "%d", mplayer_stat->PlayErrorNumbers);
	} else if (key == TR069_PARAM_PlayErrorInfo) {
		strncpy(value, mplayer_stat->PlayErrorInfo[0], size - 1);
	} else if (key == TR069_PARAM_MultiPacketsLostR1Nmb) {
		snprintf(value, size, "%d", mplayer_stat->MultiPacketsLostR[0]);
	} else if (key == TR069_PARAM_MultiPacketsLostR2Nmb) {
		snprintf(value, size, "%d", mplayer_stat->MultiPacketsLostR[1]);
	} else if (key == TR069_PARAM_MultiPacketsLostR3Nmb) {
		snprintf(value, size, "%d", mplayer_stat->MultiPacketsLostR[2]);
	} else if (key == TR069_PARAM_MultiPacketsLostR4Nmb) {
		snprintf(value, size, "%d", mplayer_stat->MultiPacketsLostR[3]);
	} else if (key == TR069_PARAM_MultiPacketsLostR5Nmb) {
		snprintf(value, size, "%d", mplayer_stat->MultiPacketsLostR[4]);
	} else if (key == TR069_PARAM_VODPacketsLostR1Nmb) {
		snprintf(value, size, "%d", mplayer_stat->VODPacketsLostR[0]);
	} else if (key == TR069_PARAM_VODPacketsLostR2Nmb) {
		snprintf(value, size, "%d", mplayer_stat->VODPacketsLostR[1]);
	} else if (key == TR069_PARAM_VODPacketsLostR3Nmb) {
		snprintf(value, size, "%d", mplayer_stat->VODPacketsLostR[2]);
	} else if (key == TR069_PARAM_VODPacketsLostR4Nmb) {
		snprintf(value, size, "%d", mplayer_stat->VODPacketsLostR[3]);
	} else if (key == TR069_PARAM_VODPacketsLostR5Nmb) {
		snprintf(value, size, "%d", mplayer_stat->VODPacketsLostR[4]);
	} else if (key == TR069_PARAM_MultiBitRateR1Nmb) {
		snprintf(value, size, "%d", mplayer_stat->MultiBitRateR[0]);
	} else if (key == TR069_PARAM_MultiBitRateR2Nmb) {
		snprintf(value, size, "%d", mplayer_stat->MultiBitRateR[1]);
	} else if (key == TR069_PARAM_MultiBitRateR3Nmb) {
		snprintf(value, size, "%d", mplayer_stat->MultiBitRateR[2]);
	} else if (key == TR069_PARAM_MultiBitRateR4Nmb) {
		snprintf(value, size, "%d", mplayer_stat->MultiBitRateR[3]);
	} else if (key == TR069_PARAM_MultiBitRateR5Nmb) {
		snprintf(value, size, "%d", mplayer_stat->MultiBitRateR[4]);
	} else if (key == TR069_PARAM_VODBitRateR1Nmb) {
		snprintf(value, size, "%d", mplayer_stat->VODBitRateR[0]);
	} else if (key == TR069_PARAM_VODBitRateR2Nmb) {
		snprintf(value, size, "%d", mplayer_stat->VODBitRateR[1]);
	} else if (key == TR069_PARAM_VODBitRateR3Nmb) {
		snprintf(value, size, "%d", mplayer_stat->VODBitRateR[2]);
	} else if (key == TR069_PARAM_VODBitRateR4Nmb) {
		snprintf(value, size, "%d", mplayer_stat->VODBitRateR[3]);
	} else if (key == TR069_PARAM_VODBitRateR5Nmb) {
		snprintf(value, size, "%d", mplayer_stat->VODBitRateR[4]);
	} else if (key == TR069_PARAM_FramesLostR1Nmb) {
		snprintf(value, size, "%d", mplayer_stat->FramesLostR[0]);
	} else if (key == TR069_PARAM_FramesLostR2Nmb) {
		snprintf(value, size, "%d", mplayer_stat->FramesLostR[1]);
	} else if (key == TR069_PARAM_FramesLostR3Nmb) {
		snprintf(value, size, "%d", mplayer_stat->FramesLostR[2]);
	} else if (key == TR069_PARAM_FramesLostR4Nmb) {
		snprintf(value, size, "%d", mplayer_stat->FramesLostR[3]);
	} else if (key == TR069_PARAM_FramesLostR5Nmb) {
		snprintf(value, size, "%d", mplayer_stat->FramesLostR[4]);
	} else if (key >= TR069_PARAM_FECMultiPacketsLostR1Nmb && 
	           key <= TR069_PARAM_FECMultiPacketsLostR5Nmb) {
	    snprintf(value, size, "%d", 0);
	} else if (key >= TR069_PARAM_ARQVODPacketsLostR1Nmb &&
			   key <= TR069_PARAM_ARQVODPacketsLostR5Nmb) {
		snprintf(value, size, "%d", 0);
	} else if (key == TR069_PARAM_BufferDecNmb) {
		snprintf(value, size, "%d", 0);
	} else if (key == TR069_PARAM_BufferIncNmb) {
	    snprintf(value, size, "%d", 0);
	} else if (key == TR069_PARAM_MultiAbendNumbers) {
	    snprintf(value, size, "%d", 0);
	} else if (key == TR069_PARAM_MultiAbendUPNumbers) {
	    snprintf(value, size, "%d", 0);
	} else if (key == TR069_PARAM_VODUPAbendNumbers) {
	    snprintf(value, size, "%d", 0);
    } else if (key == TR069_PARAM_MultiRRT) {
	    snprintf(value, size, "%d", 100);
	} else if (key == TR069_PARAM_VodRRT) {
	    snprintf(value, size, "%d", 100);
	} else if (key == TR069_PARAM_HTTPRRT) {
	    snprintf(value, size, "%d", 100);
	} else {
		return -1;
	}

	return FV_OK;
}
