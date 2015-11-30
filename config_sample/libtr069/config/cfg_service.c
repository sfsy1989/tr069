#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "cfg.h"
#include "cfg_util.h"
#include "cfg_service.h"

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <asm/types.h>
#include <netinet/ether.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include "iniparser.h"

#include "n_lib.h"
#include "n_board_config.h"
#include <time.h>
#ifdef FTM_ENABLE_IPV6
#include <ifaddrs.h>
#endif
#define BUFSIZE 8192
#define MAX_INI_LINE_COUNT 256
#define CONFIG_INIFILE "config.ini"
#define PRODUCT_INIFILE "product.ini"
#define IPTV_INIFILE "iptv.ini"
#define STB_INIFILE "stb.ini"

struct route_info {
	u_int dstAddr;
	u_int srcAddr;
	u_int gateWay;
	char ifName[32];
};

static fboolean gUbootCfgParamChanged = FV_FALSE;
static int g_usb_config_line_count = 0;
static char g_usb_config_name[MAX_INI_LINE_COUNT][64] = {{0}};
static char g_usb_config_cmd[MAX_INI_LINE_COUNT][128] = {{0}};
static char g_usb_config_value[MAX_INI_LINE_COUNT][128] = {{0}};
static fboolean g_usb_config_config_inifile_found = FV_FALSE;
static fboolean g_usb_config_product_inifile_found = FV_FALSE;
static fboolean g_usb_config_iptv_inifile_found = FV_FALSE;
static fboolean g_usb_config_stb_inifile_found = FV_FALSE;

int
readNlSock (int sockFd, char *bufPtr, int seqNum, int pId)
{
	struct nlmsghdr *nlHdr;
	int readLen = 0, msgLen = 0;
	do {
		if ((readLen = recv (sockFd, bufPtr, (size_t)(BUFSIZE - msgLen), 0)) < 0) {
			cfg_error("SOCK READ: \n");
			return -1;
		}

		nlHdr = (struct nlmsghdr *)bufPtr;
		if ((NLMSG_OK (nlHdr, readLen) == 0) /*lint !e574 */
		                || (nlHdr->nlmsg_type == NLMSG_ERROR)) {
			cfg_error("Error in recieved packet\n");
			return -1;
		}

		/* Check if the its the last message */
		if (nlHdr->nlmsg_type == NLMSG_DONE) {
			break;
		} else {
			/* Else move the pointer to buffer appropriately */
			bufPtr += readLen;
			msgLen += readLen;
		}

		/* Check if its a multi part message */
		if ((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0) {
			/* return if its not */
			break;
		}
	} while ((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
	return msgLen;
}


void
parseRoutes (struct nlmsghdr *nlHdr, struct route_info *rtInfo, char *gateway)
{
	struct rtmsg *rtMsg;
	struct rtattr *rtAttr;
	int rtLen;
	char *tempBuf = NULL;
	//2007-12-10
	struct in_addr dst;
	struct in_addr gate;

	tempBuf = (char *)malloc (100);
	rtMsg = (struct rtmsg *)NLMSG_DATA (nlHdr);
	// If the route is not for AF_INET or does not belong to main routing table
	//then return.
	if ((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN)) {
		free (tempBuf);
		return;
	}
	/* get the rtattr field */
	rtAttr = (struct rtattr *)RTM_RTA (rtMsg);
	rtLen = RTM_PAYLOAD (nlHdr);
	for (; RTA_OK (rtAttr, rtLen); rtAttr = RTA_NEXT (rtAttr, rtLen)) {
		switch (rtAttr->rta_type) {
		case RTA_OIF:
			if_indextoname (*(int *)RTA_DATA (rtAttr), rtInfo->ifName);
			break;
		case RTA_GATEWAY:
			rtInfo->gateWay = *(u_int *) RTA_DATA (rtAttr);
			break;
		case RTA_PREFSRC:
			rtInfo->srcAddr = *(u_int *) RTA_DATA (rtAttr);
			break;
		case RTA_DST:
			rtInfo->dstAddr = *(u_int *) RTA_DATA (rtAttr);
			break;
		}
	}
	//2007-12-10
	dst.s_addr = rtInfo->dstAddr;   /*lint  !e632*/
	if (strstr ((char *)inet_ntoa (dst), "0.0.0.0")) {
		gate.s_addr = rtInfo->gateWay;  /*lint  !e632*/
		sprintf (gateway, (char *)inet_ntoa (gate));
	}
	free (tempBuf);
	return;
}

#ifdef FTM_ENABLE_IPV6
static int
get_real_ip(char*nmode, char *ip)
{
	struct ifaddrs *ifap0, *ifap;
	char buf[NI_MAXHOST];
	struct sockaddr_in *addr4;
	struct sockaddr_in6 *addr6;
	int family;
	char *IPV6_FLAG=NULL;
	IPV6_FLAG = cfg_get_param_string(F_CFG_IPV6);

	if (strcmp(IPV6_FLAG, "0") == 0){
		family=AF_INET;
	}else{
		family=AF_INET6;
	}

	if(NULL == ip) {
		//strcpy (ip, "");
		return -1;
	}

	if(getifaddrs(&ifap0)) {
		return -1;
	}

	for(ifap=ifap0;ifap!=NULL;ifap=ifap->ifa_next){
		if(strcmp(nmode, ifap->ifa_name) != 0) continue;

		if(ifap->ifa_addr == NULL) continue;

		if ((ifap->ifa_flags & IFF_UP) == 0) continue;

		if(family != ifap->ifa_addr->sa_family) continue;

		if(AF_INET == ifap->ifa_addr->sa_family) {
			addr4 = (struct sockaddr_in *)ifap->ifa_addr;
			if ( NULL != inet_ntop(ifap->ifa_addr->sa_family,
			(void *)&(addr4->sin_addr), buf, NI_MAXHOST) ){
			strcpy(ip, buf);
			cfg_debug ("iface %s's ipaddress is %s\n", nmode, ip);
			freeifaddrs(ifap0);
			return 0;
			}
			else break;
		}else if(AF_INET6 == ifap->ifa_addr->sa_family) {
			addr6 = (struct sockaddr_in6 *)ifap->ifa_addr;

			if(IN6_IS_ADDR_MULTICAST(&addr6->sin6_addr)){
			continue;
			}
			if(IN6_IS_ADDR_LINKLOCAL(&addr6->sin6_addr)){
			continue;
			}
			if(IN6_IS_ADDR_LOOPBACK(&addr6->sin6_addr)){
			continue;
			}
			if(IN6_IS_ADDR_UNSPECIFIED(&addr6->sin6_addr)){
			continue;
			}
			if(IN6_IS_ADDR_SITELOCAL(&addr6->sin6_addr)){
			continue;
			}

			if(NULL != inet_ntop(ifap->ifa_addr->sa_family,
				(void *)&(addr6->sin6_addr), buf, NI_MAXHOST)){
				strcpy(ip, buf);
				cfg_debug ("iface %s's ipaddress is %s\n", nmode, ip);
				freeifaddrs(ifap0);
				return 0;
			}
				else break;
		}
	}

	freeifaddrs(ifap0);
	return -1;
}

#else
static int
get_real_ip (char *nmode, char *ip)
{
	int sock;
	struct sockaddr_in sin;
	struct ifreq ifr;

	sock = socket (AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		strcpy (ip, "");
	}
	strncpy (ifr.ifr_name, nmode, 16);
	ifr.ifr_name[15] = 0;

	if (ip != NULL) {
		if (ioctl (sock, SIOCGIFADDR, &ifr) < 0) {
			strcpy (ip, "");
		}
		memcpy (&sin, &ifr.ifr_addr, sizeof (sin));
		strncpy (ip, inet_ntoa (sin.sin_addr), 16);
		cfg_debug ("iface %s's ipaddress is %s\n", nmode, ip);
	}
	if (sock >= 0) {
		close (sock);
	}
	return 0;
}
#endif
static int
get_real_netmask (char *nmode, char *mask)
{
	int sock;
	struct ifreq ifr;
	char *IPV6_FLAG=NULL;
	IPV6_FLAG = cfg_get_param_string(F_CFG_IPV6);
	if (strcmp (IPV6_FLAG, "0") == 0){
	sock = socket (AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		strcpy (mask, "");
	}
	strncpy (ifr.ifr_name, nmode, 16);
	ifr.ifr_name[15] = 0;
	if (mask != NULL) {
		if (ioctl (sock, SIOCGIFNETMASK, &ifr) < 0) {
			strcpy (mask, "");
		}
		strcpy (mask, inet_ntoa (((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr));
		cfg_debug ("iface %s's netmask is %s\n", nmode, mask);
	}

	if (sock >= 0) {
		close (sock);
	}
		}else{
		strcpy (mask, "");
		}
	return 0;
}

int
get_gateway (char *gateway)
{
	struct nlmsghdr *nlMsg;
	//struct rtmsg *rtMsg;
	struct route_info *rtInfo;
	char msgBuf[BUFSIZE];

	int sock, len, msgSeq = 0;
	//?? Socket
	if ((sock = socket (PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
		cfg_error("Socket Creation: \n");
		return -1;
	}

	/* Initialize the buffer */
	memset (msgBuf, 0, BUFSIZE);

	/* point the header and the msg structure pointers into the buffer */
	nlMsg = (struct nlmsghdr *)msgBuf;
	//rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

	/* Fill in the nlmsg header */
	nlMsg->nlmsg_len = NLMSG_LENGTH (sizeof (struct rtmsg)); // Length of message.
	nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .

	nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
	nlMsg->nlmsg_seq = msgSeq++;/*lint !e632*/
	// Sequence of the message packet.

	nlMsg->nlmsg_pid = getpid ();/*lint !e632*/
// PID of process sending the request.

	/* Send the request */
	if (send (sock, nlMsg, (size_t)(nlMsg->nlmsg_len), 0) < 0) {
		cfg_error("Write To Socket Failed...\n");
		return -1;
	}

	/* Read the response */
	if ((len = readNlSock (sock, msgBuf, msgSeq, getpid ())) < 0) {
		cfg_error("Read From Socket Failed...\n");
		return -1;
	}
	/* Parse and print the response */
	rtInfo = (struct route_info *)malloc (sizeof (struct route_info));
	if (NULL != rtInfo) {
		for (; NLMSG_OK (nlMsg, len); nlMsg = NLMSG_NEXT (nlMsg, len)) { /*lint !e574 */
			/*lint !e574 */
			memset (rtInfo, 0, sizeof (struct route_info));
			parseRoutes (nlMsg, rtInfo, gateway);
		}
		free (rtInfo);
	}
	close (sock);
	return 0;
}

int cfg_get_real_ip(char *value, int *found)
{
	FILE *fp;
	char *addresstype_s = NULL;
	char addresstype[F_ADDRESSING_TYPE_SIZE] = {0};
	char *wireless_s = NULL;
	char *def = "";
	int ret;
	char temps[32] = {0};

	strcpy (value, def);
	*found = 1;
	/* fixme: use one address type source */
	fp = fopen ("/var/net_mode", "r");
	if (fp != NULL) {
		fscanf (fp, "%s", addresstype);
		fclose (fp);
	} else {
		addresstype_s = cfg_get_param_string(F_CFG_AddressingType);
		if (NULL == addresstype_s) {
			cfg_error("cfg_get_param_string(F_CFG_AddressingType) failed.\n");
			return -1;
		}
	}

	wireless_s = cfg_get_param_string(F_CFG_Wireless);
	if (NULL == wireless_s) {
		cfg_error("cfg_get_param_string(F_CFG_Wireless) failed.\n");
		return -1;
	}

	if (fp != NULL) {
		ret = strcmp(addresstype, "PPPOE");
	} else {
		ret = strcmp(addresstype_s, "PPPOE");
	}

	if (ret != 0) {
		if (strcmp (wireless_s, "1") == 0) {
			if (FV_OK != FV_OK) {//fixmeo_get_wireless_dev_name(temps, sizeof(temps))) {
				cfg_debug("cfg get wireless device interface is null\n");
				return -1;
			}
			get_real_ip (temps, value);
		} else {
			get_real_ip ("eth0", value);
		}
	} else
		get_real_ip ("ppp0", value);

	return FV_OK;
}

int cfg_get_real_subnetmask(char *value, int *found)
{
	FILE *fp;
	char *addresstype_s = NULL;
	char addresstype[F_ADDRESSING_TYPE_SIZE] = {0};
	char *wireless_s = NULL;
	char *def = "";
	int ret;
	char temps[32] = {0};

	strcpy (value, def);
	*found = 1;
	/* fixme: use one address type source */
	fp = fopen ("/var/net_mode", "r");
	if (fp != NULL) {
		fscanf (fp, "%s", addresstype);
		fclose (fp);
	} else {
		addresstype_s = cfg_get_param_string(F_CFG_AddressingType);
		if (NULL == addresstype_s) {
			cfg_error("cfg_get_param_string(F_CFG_AddressingType) failed.\n");
			return -1;
		}
	}

	wireless_s = cfg_get_param_string(F_CFG_Wireless);
	if (NULL == wireless_s) {
		cfg_error("cfg_get_param_string(F_CFG_Wireless) failed.\n");
		return -1;
	}

	if (fp != NULL) {
		ret = strcmp(addresstype, "PPPOE");
	} else {
		ret = strcmp(addresstype_s, "PPPOE");
	}

	if (ret != 0) {
		if (strcmp (wireless_s, "1") == 0) {
			if (FV_OK != FV_OK) {//fixmeo_get_wireless_dev_name(temps, sizeof(temps))) {
				cfg_debug("cfg get wireless device interface is null\n");
				return -1;
			}
			get_real_netmask (temps, value);
		} else {
			get_real_netmask ("eth0", value);
		}
	} else
		get_real_netmask ("ppp0", value);

	return FV_OK;
}


int cfg_get_real_gateway(char *value, int *found)
{
	char *def = "";

	strcpy (value, def);
	*found = 1;
	int gw_ret = get_gateway (value);
	if (gw_ret != 0)
		*found = 0;

	return FV_OK;
}

int cfg_get_real_dns(char *value, int *found, int index_i)
{
	FILE *fp;
	char linebuf[80], nameserver[20];
	int ret, i = 0;
	char *def = "";

	strcpy (value, def);
	fp = fopen ("/etc/resolv.conf", "r");
	if (fp == NULL) {
		*found = 0;
		return FV_OK;
	}

	while (NULL != fgets (linebuf, sizeof (linebuf), fp)) {
		ret = sscanf (linebuf, "nameserver %s", nameserver); /*FIXME:make sure pppoe resolv.conf has the same format */
		if (ret != 1)
			continue;
		i++;

		if ((index_i == 0 && 1 == i) || (index_i == 1 && 2 == i)) {
			*found = 1;
			fclose (fp);
			strcpy (value, nameserver);
			return FV_OK;
		}
	}
	*found = 0;
	fclose (fp);
	return FV_OK;
}

int cfg_service_get_net_config(int config_cmd, int config_size, char *config_value)
{
	int found = 0;
	char *value = NULL;
	char *find = NULL;

	switch (config_cmd) {
	case F_CFG_IPAddressReal: {
		cfg_get_real_ip(config_value, &found);
		break;
	}
	case F_CFG_SubnetMaskReal: {
		cfg_get_real_subnetmask(config_value, &found);
		break;
	}
	case F_CFG_DefaultGatewayReal: {
		cfg_get_real_gateway(config_value, &found);
		break;
	}
	case F_CFG_DNSServer0Real: {
		cfg_get_real_dns(config_value, &found, 0);
		break;
	}
	case F_CFG_DNSServer1Real: {
		cfg_get_real_dns(config_value, &found, 1);
		break;
	}
	case F_CFG_UpgradeDomain: {
		value = cfg_get_param_string(F_CFG_UpgradeDomain);

		find = strchr(value, ':');

		if ((NULL == value) || (NULL == find)
		                || (0 == strcmp(value, ""))
		                || (strlen(find) <= 3)) {
			value = cfg_get_param_string(F_CFG_LocalUpgradeUrl);
			cfg_set_param_string(F_CFG_UpgradeDomain, value);
		}

		/* not safe */
		strcpy(config_value, value);

		break;
	}
	default: {
		cfg_debug("no such net config value.\n");
		break;
	}
	}

	return FV_OK;
}

int cfg_service_get_config(int config_cmd, int config_size, char *config_value)
{
	int result = -1;
	char *tmp = NULL;
	char decodeStr[F_MAX_PASSWORD_SIZE] = {0};

	if((config_cmd < 0) || (config_cmd >= F_CFG_NUM)) {
		cfg_error("cmd num %d beyond\n", config_cmd);
		strcpy(config_value, "");
		return -1;
	}

	if(config_size <= cfg_get_param_string_len(config_cmd)) {
		cfg_error("config_size is %d, g_cfg_params_info[%d].bufsize is %d\n",
			config_size, config_cmd, cfg_get_ini_bufsize(config_cmd));
		strcpy(config_value, "");
		return -1;
	}

	/* add for get stbid and mac address */
	switch (config_cmd) {
	case F_CFG_FramebufferWidth:
		snprintf(config_value, config_size, "%d", n_stb_get_screen_width());
		break;
	case F_CFG_FramebufferHeight:
		snprintf(config_value, config_size, "%d", n_stb_get_screen_height());
		break;
	case F_CFG_VideoMode:
		if (n_bcfg_get_int(N_BCFG_NORM, &result) == FV_OK) {
			if (result == 0) {
				snprintf(config_value, config_size, "NTSC");
			} else {
				snprintf(config_value, config_size, "PAL");
			}
		}
		else {
			strcpy(config_value, cfg_get_param_string(config_cmd));
		}
		break;
	case F_CFG_PPPoEPassword:
	case F_CFG_DhcpPassword:
	case F_CFG_UserPassword:
		tmp = cfg_get_param_string(config_cmd);
		f_decode_chars(tmp, decodeStr, strlen(tmp));
		strcpy(config_value, decodeStr);
		break;
	case F_CFG_IPAddressReal:
	case F_CFG_SubnetMaskReal:
	case F_CFG_DefaultGatewayReal:
	case F_CFG_DNSServer0Real:
	case F_CFG_DNSServer1Real:
	case F_CFG_UpgradeDomain:
		cfg_service_get_net_config(config_cmd, config_size, config_value);
		break;
	default:
		strcpy(config_value, cfg_get_param_string(config_cmd));
		/*if (F_CFG_ManageSerURL == config_cmd) {
			strncpy(config_value, "http://172.16.15.18/acs_sim/acs.php", config_size);
			printf("### F_CFG_ManageSerURL = %s\n", config_value);
		}*/
		break;
	}

	cfg_debug("value=%s\n", config_value);
	return FV_OK;
}

int cfg_service_set_config(int config_cmd, char *config_value)
{
	char encodeStr[F_MAX_PASSWORD_SIZE] = {0};
	int value = -1;
	int hdmi_mode = 0;
	int component_mode = 0;

	if((config_cmd < 0) || (config_cmd >= F_CFG_NUM)) {
		cfg_error("cmd num %d beyond\n", config_cmd);
		return -1;
	}

	if(strlen(config_value) >= (unsigned int)cfg_get_ini_bufsize(config_cmd)) {
		cfg_error("set value len is too long\n");
		return -1;
	}

	switch (config_cmd) {
	case F_CFG_HdmiMode:
		if (NULL == config_value) {
			sprintf(config_value, O_DISP_720P50_STR);
		}

		cfg_debug("config hdmi mode is %s\n", config_value);
		cfg_set_param_string(config_cmd, config_value);

		if (!strcmp(config_value, O_DISP_720P50_STR)) {
			hdmi_mode= N_BOARD_HDMI_INDEX_720p50;
			component_mode = N_BOARD_COMPONENT_INDEX_720p50;
		} else if (!strcmp(config_value, O_DISP_720P60_STR)) {
			hdmi_mode = N_BOARD_HDMI_INDEX_720p59;
			component_mode = N_BOARD_COMPONENT_INDEX_720p59;
		} else if (!strcmp(config_value, O_DISP_1080P50_STR)) {
			hdmi_mode = N_BOARD_HDMI_INDEX_1080p50;
			component_mode = N_BOARD_COMPONENT_INDEX_1080p50;
		} else if (!strcmp(config_value, O_DISP_1080P60_STR)) {
			hdmi_mode = N_BOARD_HDMI_INDEX_1080p59;
			component_mode = N_BOARD_COMPONENT_INDEX_1080p59;
		} else {
			hdmi_mode = N_BOARD_HDMI_INDEX_720p59;
			component_mode = N_BOARD_COMPONENT_INDEX_720p59;
		}
		cfg_debug("hdmi_mode=%d,component_mode=%d\n",
			  hdmi_mode,
			  component_mode);
		n_bcfg_set_int(N_BCFG_HDMI_MODE, hdmi_mode);
		n_bcfg_set_int(N_BCFG_COMPONENT_MODE, component_mode);

		break;
	case F_CFG_HardwareVersion:
		n_bcfg_set_string(N_BCFG_HW_VERSION, config_value);

		break;
	case F_CFG_VideoMode:
		if(!strcmp(config_value, "NTSC")) {
			value = N_BCFG_NORM_NTSC;
		} else {
			value = N_BCFG_NORM_PAL;
		}
		n_bcfg_set_int(N_BCFG_NORM, value);

		cfg_set_param_string(config_cmd, config_value);

		if (N_BCFG_NORM_PAL == value) {
			cfg_set_param_string(F_CFG_FramebufferHeight, "576");
		} else if (N_BCFG_NORM_NTSC == value) {
			cfg_set_param_string(F_CFG_FramebufferHeight, "480");
		}

		gUbootCfgParamChanged = FV_TRUE;
		break;
	case F_CFG_PPPoEPassword:
	case F_CFG_DhcpPassword:
	case F_CFG_UserPassword:
		f_encode_chars(config_value, encodeStr, strlen(config_value));
		cfg_set_param_string(config_cmd, encodeStr);

		break;
	case F_CFG_MP_Volume:
		cfg_set_dynamic_param_string(config_cmd, config_value);

		break;
	default:
		if (strcmp(cfg_get_param_string(config_cmd), config_value) != 0) {
			cfg_set_param_flag(config_cmd, 1);
		}
		cfg_set_param_string(config_cmd, config_value);
		break;
	}

	return FV_OK;
}

int cfg_service_set_attribute(char *config_cmds)
{
	int i;

	for (i = 0; i < F_CFG_NUM; i++)	{
		if (config_cmds[i] != '2') {
			cfg_set_param_attribute(i, config_cmds[i]);
		}
	}

	return FV_OK;
}

int cfg_service_sync_config(int config_cmd, char *config_value, int value_len)
{
	if((config_cmd < 0) || (config_cmd >= F_CFG_NUM)) {
		cfg_error("cmd num %d beyond\n", config_cmd);
		return -1;
	}

	if(value_len >= cfg_get_ini_bufsize(config_cmd)) {
		cfg_error("set value len is too long\n");
		return -1;
	}

	cfg_params_sync(config_cmd, config_value);
	return FV_OK;
}


int cfg_service_sync_tr069(int config_cmd, char *config_value, int value_len)
{
	if((config_cmd < 0) || (config_cmd >= F_CFG_NUM)) {
		cfg_error("cmd num %d beyond\n", config_cmd);
		return -1;
	}

	if(value_len >= cfg_get_ini_bufsize(config_cmd)) {
		cfg_error("set value len is too long\n");
		return -1;
	}

	cfg_tr069_sync(config_cmd, config_value);
	return FV_OK;
}

int cfg_service_save_config(void)
{
	cfg_params_save();
	if (gUbootCfgParamChanged) {
		gUbootCfgParamChanged = FV_FALSE;
		n_bcfg_save(N_BCFG_SAVE_ENV);
	}
	return FV_OK;
}

int cfg_service_save_tr069()
{
	cfg_tr069_save();
	return FV_OK;
}

int cfg_service_save_iptv()
{
	cfg_iptv_save();
	return FV_OK;
}

int cfg_service_save_stb_info()
{
	cfg_stb_info_save();
	return FV_OK;
}

int cfg_service_save_product()
{
	cfg_product_save();
	return FV_OK;
}

int cfg_service_reload_config(void)
{
	cfg_params_load();
	return FV_OK;
}

int cfg_service_factory_reset(void)
{
	system ("cp " N_STB_PARAM_PATH "/default_config.ini " N_STB_PARAM_PATH "/config.ini");
	sync();
	return FV_OK;
}

static int ini_dump(dictionary *ini, char *stb_ini_file_path)
{
	char *tmp_file = "/var/change.ini";
	FILE *fp;

	char cmd[256] = { 0 };
	int size;

	fp = fopen(tmp_file, "w");
	if (NULL == fp) {
		cfg_error("fopen file %s failed\n", tmp_file);
		return -1;
	}

	iniparser_dump_ini(ini, fp);
	iniparser_freedict(ini);
	fseek(fp, SEEK_SET, SEEK_END);
	size = ftell(fp);
	fclose(fp);
	if (size > 10) {
		sync();
		sprintf(cmd, "mv %s %s", tmp_file, stb_ini_file_path);
		system (cmd);
		cfg_notice("%s\n", cmd);
		sync();
		n_module_stop(N_MOD_BROWS, 9);
		n_module_stop(N_MOD_MPLAYER, 9);
	} else {
		cfg_notice("%s error size=%d\n", cmd, size);
		unlink(tmp_file);
		sync();
	}

	return FV_OK;
}

static int check_ini_in_usb_config()
{
	int i = 0;

	for (i = 0; i < g_usb_config_line_count; i++) {
		if (strncmp(g_usb_config_name[i], CONFIG_INIFILE, sizeof(CONFIG_INIFILE)) == 0) {
			cfg_debug("set CONFIG_INIFILE\n");
			g_usb_config_config_inifile_found = FV_TRUE;
		} else if (strncmp(g_usb_config_name[i], PRODUCT_INIFILE, sizeof(PRODUCT_INIFILE)) == 0) {
			cfg_debug("set PRODUCT_INIFILE\n");
			g_usb_config_product_inifile_found = FV_TRUE;
		} else if (strncmp(g_usb_config_name[i], IPTV_INIFILE, sizeof(IPTV_INIFILE)) == 0) {
			cfg_debug("set IPTV_INIFILE\n");
			g_usb_config_iptv_inifile_found = FV_TRUE;
		} else if (strncmp(g_usb_config_name[i], STB_INIFILE, sizeof(STB_INIFILE)) == 0) {
			cfg_debug("set STB_INIFILE\n");
			g_usb_config_stb_inifile_found = FV_TRUE;
		}
	}
	return FV_TRUE;
}

static int change_to_usb_config(char *stb_inipath)
{
	int j = 0;
	dictionary *ini = NULL;

	cfg_debug("%s\n", stb_inipath);
	ini = iniparser_load(stb_inipath);
	for (j = 0; j < g_usb_config_line_count; j++) {
		if (strstr(stb_inipath, g_usb_config_name[j])) {
			cfg_debug("set %s to %s in %s\n",
				g_usb_config_cmd[j], g_usb_config_value[j], stb_inipath);
			iniparser_set(ini, g_usb_config_cmd[j], g_usb_config_value[j]);
		}
	}
	ini_dump(ini, stb_inipath);
	return FV_OK;
}

int cfg_service_copy_ini_file( int which_usb_ini_file)
{
	if (which_usb_ini_file == FOUND_USB0_INI_FILE) {
		system("cp /usb/disk0/fonsview/stb/*.ini /paras/");
	} else if (which_usb_ini_file == FOUND_USB1_INI_FILE) {
		system("cp /usb/disk1/fonsview/stb/*.ini /paras/");
	}
	system("cp /param/config.ini /paras/default_config.ini");
	sync();

	return FV_OK;
}

int cfg_service_read_usb_ini(char *usb_ini_file_path)
{
	FILE *f = NULL;
	int i = 0;

	cfg_debug("usb_ini_file_path=%s\n", usb_ini_file_path);

	f = fopen(usb_ini_file_path, "r");
	if (NULL == f) {
		cfg_error("open usb ini failed!\n");
		return -1;
	}

	while (fscanf(f, "%s %s %s", g_usb_config_name[i], g_usb_config_cmd[i], g_usb_config_value[i]) != EOF) {
		if (i >= MAX_INI_LINE_COUNT) {
			cfg_error("too many line of config.txt!\n");
			break;
		}

		cfg_debug("%s %s %s\n", g_usb_config_name[i], g_usb_config_cmd[i], g_usb_config_value[i]);
		i++;
		g_usb_config_line_count = i;
	}
	fclose(f);

	check_ini_in_usb_config();

	if (g_usb_config_config_inifile_found == FV_TRUE) {
		change_to_usb_config(N_STB_PARAM_PATH "/" CONFIG_INIFILE);
	}

	if (g_usb_config_product_inifile_found == FV_TRUE) {
		change_to_usb_config(N_STB_PARAM_PATH "/" PRODUCT_INIFILE);
	}

	if (g_usb_config_iptv_inifile_found == FV_TRUE) {
		change_to_usb_config(N_STB_PARAM_PATH "/" IPTV_INIFILE);
	}

	if (g_usb_config_stb_inifile_found == FV_TRUE) {
		change_to_usb_config(N_STB_PARAM_PATH "/" STB_INIFILE);
	}

	return FV_OK;
}
