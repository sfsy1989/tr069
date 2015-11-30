#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "n_lib.h"
#include "cfg.h"
#include "cfg_util.h"
#include "cfg_file.h"

#define MAX_CFG_VAL_SIZE    512
#define STBVER_FILE_PATH "/ipstb/etc/stb_ver"
#define CFG_CONFIG_INI_VERSION		"2"

typedef union {
	char   *pval;
	int    ival;
	double dval;
} cfg_val_u;

typedef struct {
	int type;
	int size;
	cfg_val_u val;
} cfg_param_s;

static int cfg_para_tr069_notify(int filetype);
static int cfg_para_notify_proc(void *args);

char g_tr069_notify[F_CFG_NUM + 1];	

static dictionary *g_ini_handle[INI_FILE_TOTAL] = {NULL};



unsigned int g_cfg_para_notify_timer = 0;

static cfg_param_s g_cfg_params[F_CFG_NUM];
static char * paramsbuf = NULL;
static cfg_params_info_s g_cfg_params_info[F_CFG_NUM] = {
	[F_CFG_CONFIG_INI_VERSION] = {CONFIG_INI_FILE, "config:version", CFG_CONFIG_INI_VERSION, F_MAX_VERSION_NUM_SIZE, '0', 0},
	[F_CFG_PRODUCT_INI_VERSION] = {PRODUCT_INI_FILE, "config:version", "1", F_MAX_VERSION_NUM_SIZE, '0', 0},
	[F_CFG_ProductClass] = {PRODUCT_INI_FILE, "Product:ProductClass", "HG000", F_TR069_DEVINFO_PRODUCT_CLASS_NAME_LEN + 1, '0', 0},
	[F_CFG_Manufacturer] = {PRODUCT_INI_FILE, "Product:Manufacturer", "FIBERHOME", F_TR069_DEVINFO_MANUFACTURER_NAME_LEN + 1, '0', 0},
	[F_CFG_ManufacturerOUI] = {PRODUCT_INI_FILE, "Product:ManufacturerOUI", "000000", F_TR069_DEVINFO_MANUFACTURER_OUI_LEN + 1, '0', 0},
	[F_CFG_SerialNumber] = {PRODUCT_INI_FILE, "Product:SerialNumber", "000000000001", F_TR069_DEVINFO_SN_LEN + 1, '0', 0},
	[F_CFG_ProvisioningCode] = {PRODUCT_INI_FILE, "Product:ProvisioningCode", "SomeCode", F_TR069_DEVINFO_PROV_CODE_LEN + 1, '0', 0},
	[F_CFG_DeviceStatus] = {PRODUCT_INI_FILE, "Product:DeviceStatus", "Up", F_TR069_DEVINFO_STATUS_LEN + 1, '0', 0},
	[F_CFG_HardwareVersion] = {PRODUCT_INI_FILE, "Product:HardwareVersion","",F_MAX_VERSION_NUM_SIZE, '0', 0},
	[F_CFG_IR_TYPE] = {PRODUCT_INI_FILE, "Device:ir", "fh2", F_MAX_TYPE_SIZE, '0', 0},
	[F_CFG_MP_Volume] = {CONFIG_DYNAMIC_INI_FILE, "MediaPlayer:Volume", "50", F_MAX_VOLUME_STR_SIZE, '0', 0},
	[F_CFG_STB_INI_VERSION] = {STB_INI_FILE, "config:version", "1", F_MAX_VERSION_NUM_SIZE, '0', 0},
	[F_CFG_MACAddress] = {STB_INI_FILE, "StbInfo:MacAddress", F_DEFAULT_MAC_ADDRESS, F_MAX_MAC_ADDR_SIZE, '0', 0},
	[F_CFG_STBID] = {STB_INI_FILE, "StbInfo:StbID", F_DEFAULT_STBID, F_IPTV_STBID_LEN + 1, '0', 0},
	[F_CFG_IPTV_INI_VERSION] = {IPTV_INI_FILE, "config:version", "1", F_MAX_VERSION_NUM_SIZE, '0', 0},
	[F_CFG_IPTV_STANDARD] = {IPTV_INI_FILE, "IPTV:standard", "IPTVS_DEFAULT", F_MAX_VERSION_NUM_SIZE, '0', 0},
	[F_CFG_OperatorInfo] = {IPTV_INI_FILE, "X_CU_STB:STBInfoOperatorInfo", "", F_IPTV_INFO_STR_SIZE, '0', 0},
	[F_CFG_AdministratorPassword] = {IPTV_INI_FILE, "X_CU_STB:STBInfoAdministratorPassword", "", F_MAX_PASSWORD_SIZE, '0', 0},
	[F_CFG_CUserPassword] = {IPTV_INI_FILE, "X_CU_STB:STBInfoUserPassword", "", F_MAX_PASSWORD_SIZE, '0', 0},
	[F_CFG_UserProvince] = {IPTV_INI_FILE, "X_CU_STB:STBInfoUserProvince", "", F_TR069_STBSERV_STANDARD_LEN, '0', 0},
	[F_CFG_StreamingControlProtocols] = {CONFIG_INI_FILE, "STBService:StreamingControlProtocols", "RTSP", F_TR069_STBSERV_PROTOCOL_SIZE, '0', 0},
	[F_CFG_StreamingTransportProtocols] = {CONFIG_INI_FILE, "STBService:StreamingTransportProtocols", "TCP,RTP", F_TR069_STBSERV_PROTOCOL_SIZE, '0', 0},
	[F_CFG_StreamingTransportControlProtocols] = {CONFIG_INI_FILE, "STBService:StreamingTransportControlProtocols", "RTCP", F_TR069_STBSERV_PROTOCOL_SIZE, '0', 0},
	[F_CFG_DownloadTransportProtocols] = {CONFIG_INI_FILE, "STBService:DownloadTransportProtocols", "HTTP", F_TR069_STBSERV_PROTOCOL_SIZE, '0', 0},
	[F_CFG_MultiplexTypes] = {CONFIG_INI_FILE, "STBService:MultiplexTypes", "MPEG2-TS", F_MAX_TYPE_SIZE, '0', 0},
	[F_CFG_MaxDejitteringBufferSize] = {CONFIG_INI_FILE, "STBService:MaxDejitteringBufferSize", "1024", 16, '0', 0},
	[F_CFG_AudioStandards] = {CONFIG_INI_FILE, "STBService:AudioStandards", "MPEG2-Part3-Layer3", F_TR069_STBSERV_STANDARD_LEN + 1, '0', 0},
	[F_CFG_VideoStandards] = {CONFIG_INI_FILE, "STBService:VideoStandards", "MPEG4-Part10", F_TR069_STBSERV_STANDARD_LEN + 1, '0', 0},
	[F_CFG_ConfigVersion] = {CONFIG_INI_FILE, "Deviceinfo:ConfigVersion", "1.5.2", F_MAX_VERSION_NUM_SIZE, '0', 0},
	[F_CFG_Type] = {CONFIG_INI_FILE, "Deviceinfo:Type", "HG600A", F_MAX_TYPE_SIZE, '0', 0},
	[F_CFG_AclUsername] = {CONFIG_INI_FILE, "DeviceInfo:AclUsername", "fonsview", F_MAX_USERNAME_SIZE, '0', 0},
	[F_CFG_AclPassword] = {CONFIG_INI_FILE, "DeviceInfo:AclPassword", "hello123", F_MAX_PASSWORD_SIZE, '0', 0},
	[F_CFG_ManageSerURL] = {TR069_INI_FILE, "ManagementServer:URL", "", F_IPTV_SERURL_LEN + 1, '0', 0},
	[F_CFG_ManageSerUsername] = {TR069_INI_FILE, "ManagementServer:Username", "990002-Class1-000000000001", F_IPTV_SERUSR_NAME_LEN + 1, '0', 0},
	[F_CFG_ManageSerPassword] = {TR069_INI_FILE, "ManagementServer:Password", "STBAdmin", F_IPTV_SERPASSWD_LEN + 1, '0', 0},
	[F_CFG_ConnectionRequestURL] = {TR069_INI_FILE, "ManagementServer:ConnectionRequestURL", "", F_IPTV_SERURL_LEN + 1, '0', 0},
	[F_CFG_ConnectionRequestUsername] = {TR069_INI_FILE, "ManagementServer:ConnectionRequestUsername", "STBAdmin", F_IPTV_SERUSR_NAME_LEN + 1, '0', 0},
	[F_CFG_ConnectionRequestPassword] = {TR069_INI_FILE, "ManagementServer:ConnectionRequestPassword", "STBAdmin", F_IPTV_SERPASSWD_LEN + 1, '0', 0},
	[F_CFG_PeriodicInformEnable] = {TR069_INI_FILE, "ManagementServer:PeriodicInformEnable", "true", F_TR069_ENABLE_SIZE, '0', 0},
	[F_CFG_PeriodicInformInterval] = {TR069_INI_FILE, "ManagementServer:PeriodicInformInterval", "10000", F_MAX_INTERVAL_SIZE, '0', 0},
	[F_CFG_PeriodicInformTime] = {TR069_INI_FILE, "ManagementServer:PeriodicInformTime", "", F_IPTV_TIMESTR_SIZE, '0', 0},
	[F_CFG_UpgradesManaged] = {TR069_INI_FILE, "ManagementServer:UpgradesManaged", "", F_TR069_MANAGMENT_UPGRADE_MANAGED_SIZE, '0', 0},
	[F_CFG_ParameterKey] = {TR069_INI_FILE, "ManagementServer:ParameterKey", "", F_TR069_MANAGMENT_PARAM_KEY_LEN + 1, '0', 0},
	[F_CFG_ManageSerURLBackup] = {TR069_INI_FILE, "ManagementServer:URLBackup", "", F_IPTV_SERURL_LEN + 1, '0', 0},
	[F_CFG_UDPConnectionRequestAddress] = {TR069_INI_FILE, "ManagementServer:UDPConnectionRequestAddress", "", F_IPTV_SERURL_LEN + 1, '0', 0},
	[F_CFG_UDPConnectionRequestNtfLimit] = {TR069_INI_FILE, "ManagementServer:UDPConnectionRequestAddressNotificationLimit", "0", F_MAX_INTERVAL_SIZE, '0', 0},
	[F_CFG_STUNEnable] = {TR069_INI_FILE, "ManagementServer:STUNEnable", "true", F_TR069_ENABLE_SIZE, '0', 0},
	[F_CFG_STUNServerAddress] = {TR069_INI_FILE, "ManagementServer:STUNServerAddress", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_STUNServerPort] = {TR069_INI_FILE, "ManagementServer:STUNServerPort", "3478", F_TR069_NUMBERCNT_SIZE, '0', 0},
	[F_CFG_STUNUsername] = {TR069_INI_FILE, "ManagementServer:STUNUsername", "", F_IPTV_SERUSR_NAME_LEN + 1, '0', 0},
	[F_CFG_STUNPassword] = {TR069_INI_FILE, "ManagementServer:STUNPassword", "", F_IPTV_SERPASSWD_LEN + 1, '0', 0},
	[F_CFG_STUNMaxKeepAlivePeriod] = {TR069_INI_FILE, "ManagementServer:STUNMaximumKeepAlivePeriod", "30", F_TR069_NUMBERCNT_SIZE, '0', 0},
	[F_CFG_STUNMinKeepAlivePeriod] = {TR069_INI_FILE, "ManagementServer:STUNMinimumKeepAlivePeriod", "10", F_TR069_NUMBERCNT_SIZE, '0', 0},
	[F_CFG_NTPServer1] = {CONFIG_INI_FILE, "Time:NTPServer1", "SomeNTPServer", F_IPTV_URL_SIZE, '0', 0},
	[F_CFG_LocalTimeZone] = {CONFIG_INI_FILE, "Time:LocalTimeZone", "+08:00", F_MAX_TIME_ZONE_STR_SIZE, '0', 0},
	[F_CFG_CurrentLocalTime] = {CONFIG_INI_FILE, "Time:CurrentLocalTime", "", F_IPTV_TIMESTR_SIZE, '0', 0},
	[F_CFG_NTPServer2] = {CONFIG_INI_FILE, "Time:NTPServer2", "", F_IPTV_URL_SIZE, '0', 0},
	[F_CFG_Wireless] = {CONFIG_INI_FILE, "LAN:Wireless", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_Device] = {CONFIG_INI_FILE, "LAN:Device", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_DhcpUser] = {CONFIG_INI_FILE, "LAN:DhcpUser", "dhcpuser001", F_MAX_USERNAME_SIZE, '0', 0},
	[F_CFG_DhcpPassword] = {CONFIG_INI_FILE, "LAN:DhcpPassword", "dhcppass", F_MAX_PASSWORD_SIZE, '0', 0},
	[F_CFG_AddressingType] = {CONFIG_INI_FILE, "LAN:AddressingType", "DHCP", F_ADDRESSING_TYPE_SIZE, '0', 0},
	[F_CFG_IPAddress] = {CONFIG_INI_FILE, "LAN:IPAddress", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_IPAddressReal] = {CONFIG_INI_FILE, "LAN:IPAddressReal", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_SubnetMask] = {CONFIG_INI_FILE, "LAN:SubnetMask", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_SubnetMaskReal] = {CONFIG_INI_FILE, "LAN:SubnetMaskReal", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_DefaultGateway] = {CONFIG_INI_FILE, "LAN:DefaultGateway", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_DefaultGatewayReal] = {CONFIG_INI_FILE, "LAN:DefaultGatewayReal", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_DNSServer0] = {CONFIG_INI_FILE, "LAN:DNSServer0", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_DNSServer0Real] = {CONFIG_INI_FILE, "LAN:DNSServer0Real", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_DNSServer1] = {CONFIG_INI_FILE, "LAN:DNSServer1", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_DNSServer1Real] = {CONFIG_INI_FILE, "LAN:DNSServer1Real", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_Wireless_SSID] = {CONFIG_INI_FILE, "LAN:Wireless.SSID", "CTC", F_WIRELESS_SSID_SIZE, '0', 0},
	[F_CFG_Wireless_AUTH] = {CONFIG_INI_FILE, "LAN:Wireless.Auth", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_Wireless_Enc] = {CONFIG_INI_FILE, "LAN:Wireless.enc", "1", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_Wireless_Key] = {CONFIG_INI_FILE, "LAN:Wireless.key", "123456", F_WIRELESS_KEY_SIZE, '0', 0},
	[F_CFG_PPPoEID] = {CONFIG_INI_FILE, "X_CTC_IPTV:ServiceInfo.PPPoEID", "", F_MAX_USERNAME_SIZE, '0', 0},
	[F_CFG_PPPoEPassword] = {CONFIG_INI_FILE, "X_CTC_IPTV:ServiceInfo.PPPoEPassword", "", F_MAX_PASSWORD_SIZE, '0', 0},
	[F_CFG_UserID] = {CONFIG_INI_FILE, "X_CTC_IPTV:ServiceInfo.UserID", "", F_MAX_USERNAME_SIZE, '0', 0},
	[F_CFG_UserPassword] = {CONFIG_INI_FILE, "X_CTC_IPTV:ServiceInfo.UserPassword", "", F_MAX_PASSWORD_SIZE, '0', 0},
	[F_CFG_UserGroupNMB] = {CONFIG_INI_FILE, "X_CTC_IPTV:F_CFG_UserGroupNMB", "", F_MAX_USERNAME_SIZE, '0', 0},
	[F_CFG_AuthURL] = {CONFIG_INI_FILE, "X_CTC_IPTV:ServiceInfo.AuthURL", "", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_AuthURLBackup] = {CONFIG_INI_FILE, "X_CTC_IPTV:ServiceInfo.AuthURLBackup", "", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_TvmsgResServer] = {CONFIG_INI_FILE, "TVMSG:ResServer", "", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_TvmsgResInterval] = {CONFIG_INI_FILE, "TVMSG:ResInterval", "300", F_MAX_INTERVAL_SIZE, '0', 0},
	[F_CFG_LocalUpgradeUrl] = {CONFIG_INI_FILE, "X_CTC_IPTV:ServiceInfo.LocalUpgradeUrl", "", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_LocalLogServerUrl] = {CONFIG_INI_FILE, "X_CTC_IPTV:ServiceInfo.LocalLogServerUrl", "", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_LocalManageServerUrl] = {CONFIG_INI_FILE, "X_CTC_IPTV:ServiceInfo.LocalManageServerUrl", "", F_IPTV_SERURL_LEN + 1, '0', 0},
	[F_CFG_LogServerUrl] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.LogServerUrl", "", F_IPTV_URL_SIZE, '0', 0},
	[F_CFG_LogUploadInterval] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.LogUploadInterval", "", F_MAX_INTERVAL_SIZE, '0', 0},
	[F_CFG_LogRecordInterval] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.LogRecordInterval", "", F_MAX_INTERVAL_SIZE, '0', 0},
	[F_CFG_StatInterval] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.StatInterval", "5", F_MAX_INTERVAL_SIZE, '0', 0},
	[F_CFG_PacketsLostR1] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.PacketsLostR1", "0", F_TR069_IPTV_PACK_LOST_CNT_SIZE, '0', 0},
	[F_CFG_PacketsLostR2] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.PacketsLostR2", "1", F_TR069_IPTV_PACK_LOST_CNT_SIZE, '0', 0},
	[F_CFG_PacketsLostR3] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.PacketsLostR3", "1", F_TR069_IPTV_PACK_LOST_CNT_SIZE, '0', 0},
	[F_CFG_PacketsLostR4] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.PacketsLostR4", "3", F_TR069_IPTV_PACK_LOST_CNT_SIZE, '0', 0},
	[F_CFG_PacketsLostR5] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.PacketsLostR5", "255", F_TR069_IPTV_PACK_LOST_CNT_SIZE, '0', 0},
	[F_CFG_BitRateR1] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.BitRateR1", "100", F_TR069_IPTV_RATE_STR_SIZE, '0', 0},
	[F_CFG_BitRateR2] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.BitRateR2", "96", F_TR069_IPTV_RATE_STR_SIZE, '0', 0},
	[F_CFG_BitRateR3] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.BitRateR3", "92", F_TR069_IPTV_RATE_STR_SIZE, '0', 0},
	[F_CFG_BitRateR4] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.BitRateR4", "88", F_TR069_IPTV_RATE_STR_SIZE, '0', 0},
	[F_CFG_BitRateR5] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.BitRateR5", "88", F_TR069_IPTV_RATE_STR_SIZE, '0', 0},
	[F_CFG_FramesLostR1] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.FramesLostR1", "0", F_TR069_IPTV_FRAME_LOST_CNT_SIZE, '0', 0},
	[F_CFG_FramesLostR2] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.FramesLostR2", "3", F_TR069_IPTV_FRAME_LOST_CNT_SIZE, '0', 0},
	[F_CFG_FramesLostR3] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.FramesLostR3", "10", F_TR069_IPTV_FRAME_LOST_CNT_SIZE, '0', 0},
	[F_CFG_FramesLostR4] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.FramesLostR4", "20", F_TR069_IPTV_FRAME_LOST_CNT_SIZE, '0', 0},
	[F_CFG_FramesLostR5] = {TR069_INI_FILE, "X_CTC_IPTV:StatisticConfiguration.FramesLostR5", "255", F_TR069_IPTV_FRAME_LOST_CNT_SIZE, '0', 0},
	[F_CFG_LogMsgEnable] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.Enable", "false", F_TR069_ENABLE_SIZE, '0', 0},
	[F_CFG_LogMsgOrFile] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.MsgOrFile", "true", F_TR069_ENABLE_SIZE, '0', 0},
	[F_CFG_LogMsgFtpServer] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.LogFtpServer", "", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_LogMsgFtpUser] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.LogFtpUser", "", F_MAX_USERNAME_SIZE, '0', 0},
	[F_CFG_LogMsgFtpPassword] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.LogFtpPassword", "", F_MAX_PASSWORD_SIZE, '0', 0},
	[F_CFG_LogMsgDuration] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.Duration", "0", F_MAX_INTERVAL_SIZE, '0', 0},
	[F_CFG_LogMsgRTSPInfo] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.RTSPInfo", "DESCRIBE RTSP 1.0", F_TR069_STRING_INFO_SIZE, '0', 0},
	[F_CFG_LogMsgHTTPInfo] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.HTTPInfo", "GET HTTP 1.0", F_TR069_STRING_INFO_SIZE, '0', 0},
	[F_CFG_LogMsgIGMPInfo] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.IGMPInfo", "JOINCHAN", F_TR069_STRING_INFO_SIZE, '0', 0},
	[F_CFG_LogMsgPkgTotalOneSec] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.PkgTotalOneSec", "113", F_TR069_NUMBERCNT_SIZE, '0', 0},
	[F_CFG_LogMsgByteTotalOneSec] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.ByteTotalOneSec", "29384", F_TR069_NUMBERCNT_SIZE, '0', 0},
	[F_CFG_LogMsgPkgLostRate] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.PkgLostRate", "1", F_TR069_IPTV_RATE_STR_SIZE, '0', 0},
	[F_CFG_LogMsgAvarageRate] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.AvarageRate", "1", F_TR069_IPTV_RATE_STR_SIZE, '0', 0},
	[F_CFG_LogMsgBUFFER] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.BUFFER", "6291456", F_TR069_IPTV_RATE_STR_SIZE, '0', 0},
	[F_CFG_LogMsgERROR] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.ERROR", "all ok", F_TR069_STRING_INFO_SIZE, '0', 0},
	[F_CFG_LogMsgVendorExt] = {INI_FILE_NONE, "X_CTC_IPTV:LogMsg.VendorExt", "no more", F_TR069_STRING_INFO_SIZE, '0', 0},
	[F_CFG_UserToken] = {CONFIG_INI_FILE, "X_CTC_IPTV:UserToken", "", F_IPTV_USR_TOKEN_LEN + 1, '0', 0},
	[F_CFG_EPGDomain] = {CONFIG_INI_FILE, "X_CTC_IPTV:EPGDomain", "", F_IPTV_DOMAIN_SIZE, '0', 0},
	[F_CFG_EPGDomainBackup] = {CONFIG_INI_FILE, "X_CTC_IPTV:EPGDomainBackup", "", F_IPTV_DOMAIN_SIZE, '0', 0},
	[F_CFG_UpgradeDomain] = {CONFIG_INI_FILE, "X_CTC_IPTV:UpgradeDomain", "", F_IPTV_DOMAIN_SIZE, '0', 0},
	[F_CFG_UpgradeDomainBackup] = {CONFIG_INI_FILE, "X_CTC_IPTV:UpgradeDomainBackup", "", F_IPTV_DOMAIN_SIZE, '0', 0},
	[F_CFG_Language] = {CONFIG_INI_FILE, "DeviceInfo:Language", "0", N_MAX_LANG_SIZE, '0', 0},
	[F_CFG_DoubleMode] = {CONFIG_INI_FILE, "DeviceInfo:DoubleMode", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_LAN_PORT_MODE] = {CONFIG_INI_FILE, "DeviceInfo:LANPortMode", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_DisplayMode] = {CONFIG_INI_FILE, "DeviceInfo:DisplayMode", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_VideoMode] = {CONFIG_INI_FILE, "display:sd","PAL",F_MAX_TYPE_SIZE, '0', 0},
	[F_CFG_Log] = {CONFIG_INI_FILE, "DeviceInfo:Log", "", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_UpgradeStatus] = {CONFIG_INI_FILE, "DeviceInfo:UpgradeStatus", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_EnableAlert] = {CONFIG_INI_FILE, "DeviceInfo:EnableAlert", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_StandbyStatus] = {CONFIG_INI_FILE, "DeviceInfo:Standby", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_NetRestartStatus] = {CONFIG_INI_FILE, "DeviceInfo:NetRestart", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_LogoVersion] = {CONFIG_INI_FILE, "Logo:version", "", F_MAX_PATH_SIZE, '0', 0},
	[F_CFG_FramebufferWidth] = {CONFIG_INI_FILE, "Display:width", "720", F_MAX_TYPE_SIZE, '0', 0},
	[F_CFG_FramebufferHeight] = {CONFIG_INI_FILE, "Display:height", "576", F_MAX_TYPE_SIZE, '0', 0},
	[F_CFG_HdmiMode] = {CONFIG_INI_FILE, "Display:hd", "HDMI_720p50", F_MAX_TYPE_SIZE, '0', 0},
	[F_CFG_StandbyMode] = {CONFIG_INI_FILE, "DeviceInfo:standbymode", "1", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_FTMVERSION] = {INI_FILE_NONE, "", "", F_MAX_VERSION_NUM_SIZE, '0', 0},
	[F_CFG_BUILD_FLAG] = {INI_FILE_NONE, "", "", F_MAX_VERSION_NUM_SIZE, '0', 0},
	[F_CFG_BuildTime] = {INI_FILE_NONE, "", "", F_MAX_TIME_ZONE_STR_SIZE, '0', 0},
	[F_CFG_KernelVersion] = {INI_FILE_NONE, "KernelVersion", "", F_MAX_VERSION_NUM_SIZE, '0', 0},
	[F_CFG_FsVersion] = {INI_FILE_NONE, "FsVersion", "", F_MAX_VERSION_NUM_SIZE, '0', 0},
	[F_CFG_IPV6] = {CONFIG_INI_FILE,"LAN:IPV6","0",F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_IPV6Stateless] = {CONFIG_INI_FILE,"LAN:IPV6Stateless","0",F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_IPortalEnable] = {CONFIG_INI_FILE, "InternalPortal:Enable", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_IPortalName1] = {CONFIG_INI_FILE, "InternalPortal:Name1", "1", F_MAX_USERNAME_SIZE, '0', 0},
	[F_CFG_IPortalValue1] = {CONFIG_INI_FILE, "InternalPortal:Value1", "", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_IPortalName2] = {CONFIG_INI_FILE, "InternalPortal:Name2", "2", F_MAX_USERNAME_SIZE, '0', 0},
	[F_CFG_IPortalValue2] = {CONFIG_INI_FILE, "InternalPortal:Value2", "", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_IPortalName3] = {CONFIG_INI_FILE, "InternalPortal:Name3", "3", F_MAX_USERNAME_SIZE, '0', 0},
	[F_CFG_IPortalValue3] = {CONFIG_INI_FILE, "InternalPortal:Value3", "", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_IPortalName4] = {CONFIG_INI_FILE, "InternalPortal:Name4", "4", F_MAX_USERNAME_SIZE, '0', 0},
	[F_CFG_IPortalValue4] = {CONFIG_INI_FILE, "InternalPortal:Value4", "", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_SQMEnableEPG] = {INI_FILE_NONE, "SQM:EnableEPG", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_SQMEnableMedia] = {INI_FILE_NONE, "SQM:EnableMedia", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_SQMEnableMediaBeforeEC] = {INI_FILE_NONE, "SQM:EnableMediaBeforeEC", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_SQMEnableWarning] = {INI_FILE_NONE, "SQM:EnableWarning", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_SQMEnableTelnet] = {INI_FILE_NONE, "SQM:EnableTelnet", "0", F_MAX_STATE_STR_SIZE, '0', 0},
	[F_CFG_SQMMediaInterval] = {INI_FILE_NONE, "SQM:MediaInterval", "0", F_MAX_INTERVAL_SIZE, '0', 0},
	[F_CFG_SQMServerAddress] = {INI_FILE_NONE, "SQM:ServerAddress", "", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_SQMServerPort] = {INI_FILE_NONE, "SQM:ServerPort", "0", F_MAX_PORT_SIZE, '0', 0},
	[F_CFG_MULTICAST_IP] = {INI_FILE_NONE, "TestInfo:MultiCastIP", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_MULTICAST_PORT] = {INI_FILE_NONE, "TestInfo:MultiCastPort", "", F_MAX_PORT_SIZE, '0', 0},
	[F_CFG_MULTICAST_SRC_IP] = {INI_FILE_NONE, "TestInfo:MultiCastSrcIP", "", F_MAX_IP_ADDR_SIZE, '0', 0},
	[F_CFG_TEST_DOMIAN] = {INI_FILE_NONE, "TestInfo:TestDomain", "", F_IPTV_DOMAIN_SIZE, '0', 0},
	[F_CFG_BOOT_LOGO] = {CONFIG_INI_FILE, "Logo:boot_logo_url", "0", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_AUTH_LOGO] = {CONFIG_INI_FILE, "Logo:auth_logo_url", "0", F_MAX_URL_SIZE, '0', 0},
	[F_CFG_BootLogoVersion] = {CONFIG_INI_FILE, "Logo:boot_logo_name", "0", F_MAX_PATH_SIZE, '0', 0},
	[F_CFG_AuthLogoVersion] = {CONFIG_INI_FILE, "Logo:auth_logo_name", "0", F_MAX_PATH_SIZE, '0', 0},
	[F_CFG_PageSave] = {IPTV_INI_FILE, "Config:pagesave", "", F_MAX_PATH_SIZE, '0', 0}
};


static char *ini_file_path2[INI_FILE_TOTAL] = {
	[CONFIG_INI_FILE] = N_STB_PARAM_PATH "/config.ini",
	[CONFIG_DYNAMIC_INI_FILE] = N_STB_PARAM_PATH "/config_dynamic.ini",
	[PRODUCT_INI_FILE] = N_STB_PARAM_PATH "/product.ini",
	[STB_INI_FILE] = N_STB_PARAM_PATH "/stb.ini",
	[IPTV_INI_FILE] = N_STB_PARAM_PATH "/iptv.ini",
	[TR069_INI_FILE] = N_STB_PARAM_PATH "/tr069_config.ini"
};

int cfg_get_ini_bufsize(int id)
{
	return g_cfg_params_info[id].bufsize;
}

char *cfg_get_ini_cmd(int id)
{
	return g_cfg_params_info[id].cmd;
}

int cfg_get_param_int(int id)
{
	return g_cfg_params[id].val.ival;
}

void cfg_set_param_int(int id, int val)
{
	g_cfg_params[id].val.ival = val;
}

double cfg_get_param_double(int id)
{
	return g_cfg_params[id].val.dval;
}

void cfg_set_param_double(int id, double val)
{
	g_cfg_params[id].val.dval = val;
}

char *cfg_get_param_string(int id)
{
	return g_cfg_params[id].val.pval;
}

int cfg_get_param_string_len(int id)
{
	return strlen(g_cfg_params[id].val.pval);
}

void cfg_set_param_string(int id, char *val)
{
	strcpy(g_cfg_params[id].val.pval, val);
}

int cfg_set_dynamic_param_string(int id, char *val)
{
	strcpy(g_cfg_params[id].val.pval, val);
	cfg_dynamic_params_save();
	return FV_OK;
}

int cfg_set_param_attribute(int id, char attribute)
{
	g_cfg_params_info[id].tr069_notify = attribute;

	return FV_OK;
}

int cfg_get_param_attribute(int id)
{
	return g_cfg_params_info[id].tr069_notify;
}

int cfg_set_param_flag(int id, int value)
{
	g_cfg_params_info[id].modify_flag = value;

	return FV_OK;
}

int cfg_get_buf_total_size()
{
	int total_size = 0;
	int i = 0;

	for(i = 0; i < F_CFG_NUM; i++) {
		total_size += g_cfg_params_info[i].bufsize;
	}
	cfg_debug("total buf size is %d\n", total_size);

	return total_size;
}

int cfg_init()
{
	int total_size;
	int i = 0;

	total_size = cfg_get_buf_total_size();

	paramsbuf = malloc((size_t)total_size);
	if(NULL == paramsbuf) {
		cfg_error("cfg_init malloc paramsbuf error\n");
		return -1;
	}

	memset(g_tr069_notify, '0', sizeof(g_tr069_notify));
	memset(paramsbuf, 0x00, total_size);

	for(i = 0; i < F_CFG_NUM; i++) {
		if(i == 0) {
			g_cfg_params[i].val.pval = paramsbuf;
		} else {
			g_cfg_params[i].val.pval = g_cfg_params[i - 1].val.pval + g_cfg_params_info[i - 1].bufsize;
		}
	}

	return FV_OK;
}

int cfg_uninit()
{
	free(paramsbuf);
	paramsbuf = NULL;

	return FV_OK;
}

static int save_ini(int id)
{
	char *tmp_file = "/data/iptv/paras/change.ini";
	FILE *fp;

	char cmd[256] = { 0 };
	int size;

	cfg_debug("\n");

	fp = fopen(tmp_file, "w");
	if (NULL == fp) {
		cfg_error("fopen file %s failed\n", tmp_file);
		return -1;
	}

	cfg_notice("save config\n");
	iniparser_dump_ini(g_ini_handle[id], fp);
	iniparser_freedict(g_ini_handle[id]);
	fseek(fp, SEEK_SET, SEEK_END);
	size = ftell(fp);
	fclose(fp);
	if (size > 10) {
		sync();
		sprintf(cmd, "mv %s %s", tmp_file, ini_file_path2[id]);
		system (cmd);
		chmod(ini_file_path2[id], 0777);
		cfg_notice("%s\n", cmd);
		sync();
	} else {
		cfg_notice("%s error size=%d\n", cmd, size);
		unlink(tmp_file);
		sync();
	}

	return FV_OK;
}

static int cfg_params_load_stb_ver()
{
	FILE *fp;
	char name[128] = {0};
	char value[128] = {0};
    char line[256] ={0};
	cfg_debug("\n");

	fp = fopen(STBVER_FILE_PATH, "r");
	if (fp == NULL)	{
		cfg_error("open stb_ver file error\n");
		return -1;
	}

	fseek(fp, 0,SEEK_SET);
	while (fgets(line, sizeof(line), fp) != NULL) {
		sscanf(line, "%[^:]:%*[\t]%[^\n]%*c", name, value); 
		cfg_debug("name=%s, value=%s\n", name, value);
		if (0 == strcmp(name, "build time")) {
			snprintf(g_cfg_params[F_CFG_BuildTime].val.pval,
				 g_cfg_params_info[F_CFG_BuildTime].bufsize, "%s", value);
			cfg_debug("build time is %s\n",
				g_cfg_params[F_CFG_BuildTime].val.pval);
		} else if (0 == strcmp(name, "ftm revision")) {
			snprintf(g_cfg_params[F_CFG_FTMVERSION].val.pval,
				 g_cfg_params_info[F_CFG_FTMVERSION].bufsize, "%s", value);
			cfg_debug("ftm revision is %s\n",
				g_cfg_params[F_CFG_FTMVERSION].val.pval);
		} else if (0 == strcmp(name, "build flag")) {
			snprintf(g_cfg_params[F_CFG_BUILD_FLAG].val.pval,
				 g_cfg_params_info[F_CFG_BUILD_FLAG].bufsize, "%s", value);
			cfg_debug("build flag is %s\n",
				g_cfg_params[F_CFG_BUILD_FLAG].val.pval);					
		}
		memset(value, 0, sizeof(value)); 
		memset(name, 0, sizeof(name)); 
	}

	fclose(fp);

	return FV_OK;
}

static void load_ini_string_parameter(int param)
{
	if (NULL == g_ini_handle[g_cfg_params_info[param].ini_file]) {
		snprintf(g_cfg_params[param].val.pval, g_cfg_params_info[param].bufsize,
			"%s", g_cfg_params_info[param].default_value);
	} else if (INI_FILE_NONE == g_cfg_params_info[param].ini_file) {
		snprintf(g_cfg_params[param].val.pval, g_cfg_params_info[param].bufsize,
			"%s", g_cfg_params_info[param].default_value);		
	} else {
		snprintf(g_cfg_params[param].val.pval, g_cfg_params_info[param].bufsize,
			"%s",
			iniparser_getstring(g_ini_handle[g_cfg_params_info[param].ini_file],
				(const char *)g_cfg_params_info[param].cmd,
				g_cfg_params_info[param].default_value));
	}
}

static void save_config_ini_string_parameter(int param)
{
	if (NULL == g_ini_handle[CONFIG_INI_FILE]) {
		g_ini_handle[CONFIG_INI_FILE] = iniparser_load((char*)ini_file_path2[CONFIG_INI_FILE]);
	}

	cfg_debug("param=%d dict=0x%X cmd=%s value=%s start\n",
		param, g_ini_handle[g_cfg_params_info[param].ini_file],
		g_cfg_params_info[param].cmd, g_cfg_params[param].val.pval);
	iniparser_set(g_ini_handle[g_cfg_params_info[param].ini_file],
		g_cfg_params_info[param].cmd,
		g_cfg_params[param].val.pval);
	cfg_debug("param=%d done\n", param);
}

static void set_string_parameter(int param, char *value)
{
	snprintf(g_cfg_params[param].val.pval, g_cfg_params_info[param].bufsize, "%s", value);
}

static char* get_string_parameter(int param)
{
	return (char*)g_cfg_params[param].val.pval;
}

static int cfg_params_upgrade_v1_to_v2()
{
	char cmd[N_MAX_SHELL_CMD_SIZE];
	FILE *fp;
	int size;

	cfg_notice("++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	cfg_notice("++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	cfg_notice("\n");
	cfg_notice("++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	cfg_notice("++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

	fp = fopen("/var/config.ini", "w");
	if (NULL == fp) {
		return -1;
	}

	set_string_parameter(F_CFG_CONFIG_INI_VERSION, CFG_CONFIG_INI_VERSION);

	save_config_ini_string_parameter(F_CFG_CONFIG_INI_VERSION);
	iniparser_dump_ini(g_ini_handle[CONFIG_INI_FILE], fp);
	iniparser_freedict(g_ini_handle[CONFIG_INI_FILE]);
	g_ini_handle[CONFIG_INI_FILE] = NULL;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fclose(fp);
	system("cp " PREFIX "/share/upgrade_files/v1tov2/* " N_STB_PARAM_PATH "/");
	if (size > 10) {
		sprintf(cmd, "mv %s %s", "/var/config.ini", ini_file_path2[CONFIG_INI_FILE]);
		system(cmd);
		sync();
	} else {
		unlink("/var/config.ini");
		sync();
	}

	g_ini_handle[CONFIG_INI_FILE] = iniparser_load((char*)ini_file_path2[CONFIG_INI_FILE]);

	return FV_OK;
}

static int create_default_tr069_ini()
{
	FILE *fp;

	fp = fopen(N_STB_PARAM_PATH "/tr069_config.ini", "r");
	if (NULL == fp) {
		cfg_debug("creat tr069_config.ini file");
		system("cp " PREFIX "/share/upgrade_files/tr069_config.ini " N_STB_PARAM_PATH "/");
		sync();
	} else {
		fclose(fp);
	}
	return FV_OK;
}

static int create_default_dynamic_config_ini()
{
	FILE *fp;

	fp = fopen(N_STB_PARAM_PATH "/config_dynamic.ini", "r");
	if (NULL == fp) {
		cfg_debug("creat config_dynamic.ini file");
		system("cp " PREFIX "/share/upgrade_files/config_dynamic.ini " N_STB_PARAM_PATH "/");
		sync();
	} else {
		fclose(fp);
	}
	return FV_OK;
}

void cfg_file_is_exist()
{
	int i,ret;
	char * cmdBuf[128] = {0};
	ret = access((char*)ini_file_path2[CONFIG_INI_FILE],0);	
	if(ret == -1){			
		sprintf(cmdBuf,"busybox tar -xvf %s -C /data/;busybox chmod -R 777 /data/iptv ",N_DEF_STB_PARAM_PATH);
		system(cmdBuf);
	}	
}

int cfg_params_load()
{
	int i = 0;
	cfg_params_load_stb_ver();

	g_ini_handle[CONFIG_INI_FILE] = iniparser_load((char*)ini_file_path2[CONFIG_INI_FILE]);
	load_ini_string_parameter(F_CFG_CONFIG_INI_VERSION);
	cfg_notice("config version=%s\n", get_string_parameter(F_CFG_CONFIG_INI_VERSION));
	if (0 == strcmp("1", get_string_parameter(F_CFG_CONFIG_INI_VERSION))) {
		cfg_params_upgrade_v1_to_v2();
	}

	create_default_tr069_ini();
	create_default_dynamic_config_ini();
	
	for (i = CONFIG_INI_FILE + 1; i < INI_FILE_TOTAL; i++) {
		if(i != STB_INI_FILE){
			g_ini_handle[i] = iniparser_load((char*)ini_file_path2[i]);
		}else{
			g_ini_handle[i] = iniparser_load_stb();		
		}
		if(NULL == g_ini_handle[i]) {
				cfg_error("load %s error\n", (char *)ini_file_path2[i]);
		}
	}

	for(i = 0; i < F_CFG_NUM; i++) {
		switch(i) {
		case F_CFG_BUILD_FLAG:
		case F_CFG_FTMVERSION:
		case F_CFG_BuildTime:
			// load from cfg_params_load_stb_ver
			break;
		case F_CFG_KernelVersion:
			cfg_get_kernelversion(g_cfg_params[i].val.pval, g_cfg_params_info[i].bufsize);
			break;
		case F_CFG_FsVersion:
			cfg_get_fsversion(g_cfg_params[i].val.pval, g_cfg_params_info[i].bufsize);
			break;
		default:
			load_ini_string_parameter(i);
			break;
		}
	}

	for (i = 0; i < INI_FILE_TOTAL; i++) {
		if(g_ini_handle[i] != NULL){
			iniparser_freedict(g_ini_handle[i]);
			g_ini_handle[i] = NULL;
		}
	}

	for (i = 0; i < F_CFG_NUM; i++) {
		cfg_set_param_flag(i, 0);
	}

	return FV_OK;
}

int cfg_params_save()
{
	int i = 0;
	
	cfg_debug("\n");
	
	if(cfg_para_tr069_notify(CONFIG_INI_FILE) < 0) 
		return -1;
	
	g_ini_handle[CONFIG_INI_FILE] = iniparser_load((char*)ini_file_path2[CONFIG_INI_FILE]);
	for(i = 0; i < F_CFG_NUM; i++) {
		if (CONFIG_INI_FILE == g_cfg_params_info[i].ini_file) {
			save_config_ini_string_parameter(i);
		}

		if (F_CFG_HdmiMode == i) {
			//fixeme
			//o_disp_cfg_mod_set_format(cfg_get_param_string(F_CFG_VideoMode),
			//			  cfg_get_param_string(F_CFG_HdmiMode));
		}
	}

	save_ini(CONFIG_INI_FILE);

	return FV_OK;
}

int cfg_dynamic_params_save()
{
	int i = 0;

	cfg_debug("\n");
	
	if(cfg_para_tr069_notify(CONFIG_DYNAMIC_INI_FILE) < 0) 
		return -1;
	
	g_ini_handle[CONFIG_DYNAMIC_INI_FILE] = iniparser_load((char*)ini_file_path2[CONFIG_DYNAMIC_INI_FILE]);
	for(i = 0; i < F_CFG_NUM; i++) {
		if (CONFIG_DYNAMIC_INI_FILE == g_cfg_params_info[i].ini_file) {
			save_config_ini_string_parameter(i);
		}
	}

	save_ini(CONFIG_DYNAMIC_INI_FILE);
	return FV_OK;
}

int cfg_tr069_save()
{
	int i = 0;

	cfg_debug("\n");
	
	if(cfg_para_tr069_notify(TR069_INI_FILE) < 0) 
		return -1;
	
	g_ini_handle[TR069_INI_FILE] = iniparser_load((char*)ini_file_path2[TR069_INI_FILE]);
	for(i = 0; i < F_CFG_NUM; i++) {
		if (TR069_INI_FILE == g_cfg_params_info[i].ini_file) {
			save_config_ini_string_parameter(i);
		}
	}

	save_ini(TR069_INI_FILE);
	return FV_OK;
}

int cfg_iptv_save()
{
	int i = 0;
	
	cfg_debug("\n");
	
	if(cfg_para_tr069_notify(IPTV_INI_FILE) < 0) 
		return -1;
	
	g_ini_handle[IPTV_INI_FILE] = iniparser_load((char*)ini_file_path2[IPTV_INI_FILE]);
	for(i = 0; i < F_CFG_NUM; i++) {
		if (IPTV_INI_FILE == g_cfg_params_info[i].ini_file) {
			save_config_ini_string_parameter(i);
		}
	}

	save_ini(IPTV_INI_FILE);
	return FV_OK;
}

int cfg_product_save()
{
	int i = 0;

	cfg_debug("\n");
	
	if(cfg_para_tr069_notify(PRODUCT_INI_FILE) < 0) 
		return -1;
	
	g_ini_handle[PRODUCT_INI_FILE] = iniparser_load((char*)ini_file_path2[PRODUCT_INI_FILE]);
	for(i = 0; i < F_CFG_NUM; i++) {
		if (PRODUCT_INI_FILE == g_cfg_params_info[i].ini_file) {
			save_config_ini_string_parameter(i);
	     }
	}
	save_ini(PRODUCT_INI_FILE);
	return FV_OK;
}

int cfg_stb_info_save()
{
	int i = 0;

	cfg_debug("\n");
	
	if(cfg_para_tr069_notify(STB_INI_FILE) < 0) 
		return -1;
	
	g_ini_handle[STB_INI_FILE] = iniparser_load((char*)ini_file_path2[STB_INI_FILE]);
	for(i = 0; i < F_CFG_NUM; i++) {
		if (STB_INI_FILE == g_cfg_params_info[i].ini_file) {
			save_config_ini_string_parameter(i);
		}
	}
	save_ini(STB_INI_FILE);
	return FV_OK;
}

int cfg_params_sync(int sync_cmd, char *sync_value)
{
	dictionary *ini = NULL;
	char *file_name = NULL;
	char tmpfile_name[256] = { 0 };
	char cmd[256] = { 0 };
	int size;
	FILE *f;

	file_name = (char *)ini_file_path2[CONFIG_INI_FILE];
	sprintf (tmpfile_name, "%s.new", file_name);
	f = fopen (tmpfile_name, "w");
	if (NULL == f) {
		cfg_error ("open the save ini error!\n");
		return -1;
	}

	ini = iniparser_load (file_name);
	if(NULL == ini) {
		cfg_error("sync params to  %s error\n", file_name);
		return -1;
	}

	iniparser_set(ini, g_cfg_params_info[sync_cmd].cmd, sync_value);

	iniparser_dump_ini(ini, f);
	iniparser_freedict(ini);
	fseek(f, SEEK_SET, SEEK_END);
	size = ftell (f);
	fclose (f);
	if (size > 10) {
		sprintf (cmd, "mv %s %s", tmpfile_name, file_name);
		system (cmd);
		cfg_notice("%s\n", cmd);
		sync();
	} else {
		unlink (tmpfile_name);
		sync();
	}

	return FV_OK;
}

int cfg_tr069_sync(int sync_cmd, char *sync_value)
{
	dictionary *ini = NULL;
	char *file_name = NULL;
	char tmpfile_name[256] = { 0 };
	char cmd[256] = { 0 };
	int size;
	FILE *f;

	file_name = (char *)ini_file_path2[TR069_INI_FILE];
	sprintf (tmpfile_name, "%s.new", file_name);
	f = fopen (tmpfile_name, "w");
	if (NULL == f) {
		cfg_error ("open the save ini error!\n");
		return -1;
	}

	ini = iniparser_load (file_name);
	if(NULL == ini) {
		cfg_error("sync params to  %s error\n", file_name);
		return -1;
	}

	iniparser_set(ini, g_cfg_params_info[sync_cmd].cmd, sync_value);

	iniparser_dump_ini(ini, f);
	iniparser_freedict(ini);
	fseek(f, SEEK_SET, SEEK_END);
	size = ftell (f);
	fclose (f);
	if (size > 10) {
		printf("---------------------- %d    %s\n", sync_cmd, sync_value);
		sprintf (cmd, "mv %s %s", tmpfile_name, file_name);
		system (cmd);
		cfg_notice("%s\n", cmd);
		sync();
	} else {
		unlink (tmpfile_name);
		sync();
	}

	return FV_OK;
}


static int cfg_para_tr069_notify(int filetype)
{
	int i;
	int need_notify = 0;
	
	for(i = 0; i < F_CFG_NUM; i++) {
		if (g_cfg_params_info[i].ini_file == filetype) {
			g_tr069_notify[i] = '0';
			
			if((1 == g_cfg_params_info[i].modify_flag) && ('0' != cfg_get_param_attribute(i))) {
				g_tr069_notify[i] = '1';
				cfg_set_param_flag(i, 0);
				need_notify = 1;
			}
		}
	}
	
	g_tr069_notify[F_CFG_NUM] = '\0';
	
	if(need_notify == 1) {
		if(g_cfg_para_notify_timer == 0) {
			g_cfg_para_notify_timer = f_timer_add(1000, cfg_para_notify_proc, (void *)0);
	
			if (g_cfg_para_notify_timer <= 0) {
				cfg_error("f_timer_add failed.\n");
				return -1;
			}
		}
	}

	return 0;
}

static int cfg_para_notify_proc(void *args)
{
	if (N_MOD_RUNNING == n_mod_get_stat(N_MOD_CPE)) {
		n_tr069_value_change_notify(g_tr069_notify);
	}
	memset(g_tr069_notify,'0',sizeof(g_tr069_notify));
	g_cfg_para_notify_timer = 0;
	return -1;
}
