/********************************************************************
	Created By Ash, 2009/11/25
	Model: Tr069
	Sub Model: Parameter
	Function: Parameter management and maintenance
********************************************************************/
#include <n_lib.h>
#include <time.h>
#include "tr069_parameter.h"
#include "tr069_timer.h"
#include "tr069_interface.h"
#include "tr069.h"

#include <n_stb_tr069.h>

/* global */

/* extern */
extern int tr069_set_config(int key, char *value);
extern int tr069_get_config(int key, char *value, unsigned int size);
extern int tr069_format_time(time_t now_time, char *tr069_time, int size);
extern int g_tr069_stand;

/* local */
static VOID_T tr069_parameter_idstack_clear(tr069_parameter_mgr *param_handle);

static INT32_T tr069_parameter_idstack_push(tr069_parameter_mgr *param_handle, UINT16_T id);

static INT32_T tr069_parameter_idstack_pop(tr069_parameter_mgr *param_handle, UINT16_T *id);

static VOID_T tr069_parameter_recursive_get(tr069_parameter_mgr *param_handle, tr069_parameter *param, tr069_parameter **param_set, UINT16_T *get_num, UINT8_T object_flag);

static VOID_T tr069_parameter_recursive_remove(tr069_parameter_mgr *param_handle, tr069_parameter *param);

static INT32_T tr069_parameter_init_params(tr069_parameter_mgr *parameter_handle);

int tr069_index_map[] = {
	[TR069_PARAM_Manufacturer] = F_CFG_Manufacturer,
	[TR069_PARAM_ManufacturerOUI] = F_CFG_ManufacturerOUI,
	[TR069_PARAM_ProductClass] = F_CFG_ProductClass,
	[TR069_PARAM_SerialNumber] = F_CFG_SerialNumber,
	[TR069_PARAM_HardwareVersion] = F_CFG_HardwareVersion,
	[TR069_PARAM_SoftwareVersion] = F_CFG_FsVersion,
	[TR069_PARAM_ProvisioningCode] = F_CFG_ProvisioningCode,
	[TR069_PARAM_DeviceStatus] = F_CFG_DeviceStatus,
	[TR069_PARAM_KernelVersion] = F_CFG_KernelVersion,
	[TR069_PARAM_FsVersion] = F_CFG_FsVersion,
	[TR069_PARAM_AppVersion] = F_CFG_FsVersion,
	[TR069_PARAM_ModelName] =  F_CFG_ProductClass,
	[TR069_PARAM_ModelID] =  F_CFG_ProductClass,
	[TR069_PARAM_Description] = 0,
	[TR069_PARAM_ConfigFileVersion] = 0,
	[TR069_PARAM_UpTime] = 0,
	[TR069_PARAM_FirstUseDate] = 0,
	[TR069_PARAM_DeviceLog] = 0,
	[TR069_PARAM_StreamingControlProtocols] = F_CFG_StreamingControlProtocols,
	[TR069_PARAM_StreamingTransportProtocols] = F_CFG_StreamingTransportProtocols,
	[TR069_PARAM_StreamingTransportControlProtocols] = F_CFG_StreamingTransportControlProtocols,
	[TR069_PARAM_DownloadTransportProtocols] = F_CFG_DownloadTransportProtocols,
	[TR069_PARAM_MultiplexTypes] = F_CFG_MultiplexTypes,
	[TR069_PARAM_MaxDejitteringBufferSize] = F_CFG_MaxDejitteringBufferSize,
	[TR069_PARAM_AudioStandards] = F_CFG_AudioStandards,
	[TR069_PARAM_VideoStandards] = F_CFG_VideoStandards,
	[TR069_PARAM_ManageSerURL] = F_CFG_ManageSerURL,
	[TR069_PARAM_ManageSerUsername] = F_CFG_ManageSerUsername,
	[TR069_PARAM_ManageSerPassword] = F_CFG_ManageSerPassword,
	[TR069_PARAM_ConnectionRequestURL] = F_CFG_ConnectionRequestURL,
	[TR069_PARAM_ConnectionRequestUsername] = F_CFG_ConnectionRequestUsername,
	[TR069_PARAM_ConnectionRequestPassword] = F_CFG_ConnectionRequestPassword,
	[TR069_PARAM_PeriodicInformEnable] = F_CFG_PeriodicInformEnable,
	[TR069_PARAM_PeriodicInformInterval] = F_CFG_PeriodicInformInterval,
	[TR069_PARAM_PeriodicInformTime] = F_CFG_PeriodicInformTime,
	[TR069_PARAM_UpgradesManaged] = F_CFG_UpgradesManaged,
	[TR069_PARAM_ParameterKey] = F_CFG_ParameterKey,
	[TR069_PARAM_ManageSerURLBackup] = F_CFG_ManageSerURLBackup,
	[TR069_PARAM_UDPConnectionRequestAddress] = F_CFG_UDPConnectionRequestAddress,
	[TR069_PARAM_UDPConnectionRequestNtfLimit] = F_CFG_UDPConnectionRequestNtfLimit,
	[TR069_PARAM_STUNEnable] = F_CFG_STUNEnable,
	[TR069_PARAM_STUNServerAddress] = F_CFG_STUNServerAddress,
	[TR069_PARAM_STUNServerPort] = F_CFG_STUNServerPort,
	[TR069_PARAM_STUNUsername] = F_CFG_STUNUsername,
	[TR069_PARAM_STUNPassword] = F_CFG_STUNPassword,
	[TR069_PARAM_STUNMaxKeepAlivePeriod] = F_CFG_STUNMaxKeepAlivePeriod,
	[TR069_PARAM_STUNMinKeepAlivePeriod] = F_CFG_STUNMinKeepAlivePeriod,
	[TR069_PARAM_NATDetected] = 0,
	[TR069_PARAM_NTPServer1] = F_CFG_NTPServer1,
	[TR069_PARAM_LocalTimeZone] = F_CFG_LocalTimeZone,
	[TR069_PARAM_CurrentLocalTime] = F_CFG_CurrentLocalTime,
	[TR069_PARAM_Device] = F_CFG_Device,
	[TR069_PARAM_DhcpUser] = F_CFG_DhcpUser,
	[TR069_PARAM_DhcpPassword] = F_CFG_DhcpPassword,
	[TR069_PARAM_AddressingType] = F_CFG_AddressingType,
	[TR069_PARAM_IPAddressReal] = F_CFG_IPAddressReal,
	[TR069_PARAM_SubnetMaskReal] = F_CFG_SubnetMaskReal,
	[TR069_PARAM_DefaultGatewayReal] = F_CFG_DefaultGatewayReal,
	[TR069_PARAM_DNSServer0Real] = F_CFG_DNSServer0Real,
	[TR069_PARAM_DNSServer1Real] = F_CFG_DNSServer1Real,
	[TR069_PARAM_MACAddress] = F_CFG_MACAddress,
	[TR069_PARAM_ConnectionUpTime] = 0,
	[TR069_PARAM_TotalBytesSent] = 0,
	[TR069_PARAM_TotalBytesReceived] = 0,
	[TR069_PARAM_TotalPacketsSent] = 0,
	[TR069_PARAM_TotalPacketsReceived] = 0,
	[TR069_PARAM_CurrentDayInterval] = 0,
	[TR069_PARAM_CurrentDayBytesSent] = 0,
	[TR069_PARAM_CurrentDayBytesReceived] = 0,
	[TR069_PARAM_CurrentDayPacketsSent] = 0,
	[TR069_PARAM_CurrentDayPacketsReceived] = 0,
	[TR069_PARAM_QuarterHourInterval] = 0,
	[TR069_PARAM_QuarterHourBytesSent] = 0,
	[TR069_PARAM_QuarterHourBytesReceived] = 0,
	[TR069_PARAM_QuarterHourPacketsSent] = 0,
	[TR069_PARAM_QuarterHourPacketsReceived] = 0,
	[TR069_PARAM_PingDiagState] = 0,
	[TR069_PARAM_PingDiagHost] = 0,
	[TR069_PARAM_PingDiagReptNumber] = 0,
	[TR069_PARAM_PingDiagTimeout] = 0,
	[TR069_PARAM_PingDiagDataBlockSize] = 0,
	[TR069_PARAM_PingDiagDSCP] = 0,
	[TR069_PARAM_PingDiagSuccessCount] = 0,
	[TR069_PARAM_PingDiagFailureCount] = 0,
	[TR069_PARAM_PingDiagAvgRespTime] = 0,
	[TR069_PARAM_PingDiagMinRespTime] = 0,
	[TR069_PARAM_PingDiagMaxRespTime] = 0,
	[TR069_PARAM_TracertDiagState] = 0,
	[TR069_PARAM_TracertDiagHost] = 0,
	[TR069_PARAM_TracertDiagTimeout] = 0,
	[TR069_PARAM_TracertDiagDataBlockSize] = 0,
	[TR069_PARAM_TracertDiagMaxHopCount] = 0,
	[TR069_PARAM_TracertDiagDSCP] = 0,
	[TR069_PARAM_TracertDiagRespTime] = 0,
	[TR069_PARAM_TracertDiagHopsNumber] = 0,
	[TR069_PARAM_HOPHOST1] = 0,
	[TR069_PARAM_HOPHOST2] = 0,
	[TR069_PARAM_HOPHOST3] = 0,
	[TR069_PARAM_HOPHOST4] = 0,
	[TR069_PARAM_HOPHOST5] = 0,
	[TR069_PARAM_HOPHOST6] = 0,
	[TR069_PARAM_HOPHOST7] = 0,
	[TR069_PARAM_HOPHOST8] = 0,
	[TR069_PARAM_HOPHOST9] = 0,
	[TR069_PARAM_HOPHOST10] = 0,
	[TR069_PARAM_HOPHOST11] = 0,
	[TR069_PARAM_HOPHOST12] = 0,
	[TR069_PARAM_HOPHOST13] = 0,
	[TR069_PARAM_HOPHOST14] = 0,
	[TR069_PARAM_HOPHOST15] = 0,
	[TR069_PARAM_HOPHOST16] = 0,
	[TR069_PARAM_HOPHOST17] = 0,
	[TR069_PARAM_HOPHOST18] = 0,
	[TR069_PARAM_HOPHOST19] = 0,
	[TR069_PARAM_HOPHOST20] = 0,
	[TR069_PARAM_HOPHOST21] = 0,
	[TR069_PARAM_HOPHOST22] = 0,
	[TR069_PARAM_HOPHOST23] = 0,
	[TR069_PARAM_HOPHOST24] = 0,
	[TR069_PARAM_HOPHOST25] = 0,
	[TR069_PARAM_HOPHOST26] = 0,
	[TR069_PARAM_HOPHOST27] = 0,
	[TR069_PARAM_HOPHOST28] = 0,
	[TR069_PARAM_HOPHOST29] = 0,
	[TR069_PARAM_HOPHOST30] = 0,
	[TR069_PARAM_HOPHOST31] = 0,
	[TR069_PARAM_HOPHOST32] = 0,
	[TR069_PARAM_STBID] = F_CFG_STBID,
	[TR069_PARAM_PPPoEID] = F_CFG_PPPoEID,
	[TR069_PARAM_PPPoEPassword] = F_CFG_PPPoEPassword,
	[TR069_PARAM_UserID] = F_CFG_UserID,
	[TR069_PARAM_UserPassword] = F_CFG_UserPassword,
	[TR069_PARAM_AuthURL] = F_CFG_AuthURL,
	[TR069_PARAM_AuthURLBackup] = F_CFG_AuthURLBackup,
	[TR069_PARAM_LogServerUrl] = F_CFG_LogServerUrl,
	[TR069_PARAM_LogUploadInterval] = F_CFG_LogUploadInterval,
	[TR069_PARAM_LogRecordInterval] = F_CFG_LogRecordInterval,
	[TR069_PARAM_StatInterval] = F_CFG_StatInterval,
	[TR069_PARAM_PacketsLostR1] = F_CFG_PacketsLostR1,
	[TR069_PARAM_PacketsLostR2] = F_CFG_PacketsLostR2,
	[TR069_PARAM_PacketsLostR3] = F_CFG_PacketsLostR3,
	[TR069_PARAM_PacketsLostR4] = F_CFG_PacketsLostR4,
	[TR069_PARAM_PacketsLostR5] = F_CFG_PacketsLostR5,
	[TR069_PARAM_BitRateR1] = F_CFG_BitRateR1,
	[TR069_PARAM_BitRateR2] = F_CFG_BitRateR2,
	[TR069_PARAM_BitRateR3] = F_CFG_BitRateR3,
	[TR069_PARAM_BitRateR4] = F_CFG_BitRateR4,
	[TR069_PARAM_BitRateR5] = F_CFG_BitRateR5,
	[TR069_PARAM_FramesLostR1] = F_CFG_FramesLostR1,
	[TR069_PARAM_FramesLostR2] = F_CFG_FramesLostR2,
	[TR069_PARAM_FramesLostR3] = F_CFG_FramesLostR3,
	[TR069_PARAM_FramesLostR4] = F_CFG_FramesLostR4,
	[TR069_PARAM_FramesLostR5] = F_CFG_FramesLostR5,
	[TR069_PARAM_Startpoint] = 0,
	[TR069_PARAM_Endpoint] = 0,
	[TR069_PARAM_AuthNumbers] = 0,
	[TR069_PARAM_AuthFailNumbers] = 0,
	[TR069_PARAM_AuthFailInfo] = 0,
	[TR069_PARAM_MultiReqNumbers] = 0,
	[TR069_PARAM_MultiFailNumbers] = 0,
	[TR069_PARAM_MultiFailInfo] = 0,
	[TR069_PARAM_VodReqNumbers] = 0,
	[TR069_PARAM_VodFailNumbers] = 0,
	[TR069_PARAM_VodFailInfo] = 0,
	[TR069_PARAM_HTTPReqNumbers] = 0,
	[TR069_PARAM_HTTPFailNumbers] = 0,
	[TR069_PARAM_HTTPFailInfo] = 0,
	[TR069_PARAM_MutiAbendNumbers] = 0,
	[TR069_PARAM_VODAbendNumbers] = 0,
	[TR069_PARAM_PlayErrorNumbers] = 0,
	[TR069_PARAM_MultiPacketsLostR1Nmb] = 0,
	[TR069_PARAM_MultiPacketsLostR2Nmb] = 0,
	[TR069_PARAM_MultiPacketsLostR3Nmb] = 0,
	[TR069_PARAM_MultiPacketsLostR4Nmb] = 0,
	[TR069_PARAM_MultiPacketsLostR5Nmb] = 0,
	[TR069_PARAM_VODPacketsLostR1Nmb] = 0,
	[TR069_PARAM_VODPacketsLostR2Nmb] = 0,
	[TR069_PARAM_VODPacketsLostR3Nmb] = 0,
	[TR069_PARAM_VODPacketsLostR4Nmb] = 0,
	[TR069_PARAM_VODPacketsLostR5Nmb] = 0,
	[TR069_PARAM_MultiBitRateR1Nmb] = 0,
	[TR069_PARAM_MultiBitRateR2Nmb] = 0,
	[TR069_PARAM_MultiBitRateR3Nmb] = 0,
	[TR069_PARAM_MultiBitRateR4Nmb] = 0,
	[TR069_PARAM_MultiBitRateR5Nmb] = 0,
	[TR069_PARAM_VODBitRateR1Nmb] = 0,
	[TR069_PARAM_VODBitRateR2Nmb] = 0,
	[TR069_PARAM_VODBitRateR3Nmb] = 0,
	[TR069_PARAM_VODBitRateR4Nmb] = 0,
	[TR069_PARAM_VODBitRateR5Nmb] = 0,
	[TR069_PARAM_FramesLostR1Nmb] = 0,
	[TR069_PARAM_FramesLostR2Nmb] = 0,
	[TR069_PARAM_FramesLostR3Nmb] = 0,
	[TR069_PARAM_FramesLostR4Nmb] = 0,
	[TR069_PARAM_FramesLostR5Nmb] = 0,
	[TR069_PARAM_PlayErrorInfo] = 0,
	[TR069_PARAM_LogMsgEnable] = F_CFG_LogMsgEnable,
	[TR069_PARAM_LogMsgOrFile] = F_CFG_LogMsgOrFile,
	[TR069_PARAM_LogMsgFtpServer] = F_CFG_LogMsgFtpServer,
	[TR069_PARAM_LogMsgFtpUser] = F_CFG_LogMsgFtpUser,
	[TR069_PARAM_LogMsgFtpPassword] = F_CFG_LogMsgFtpPassword,
	[TR069_PARAM_LogMsgDuration] = F_CFG_LogMsgDuration,
	[TR069_PARAM_LogMsgRTSPInfo] = F_CFG_LogMsgRTSPInfo,
	[TR069_PARAM_LogMsgHTTPInfo] = F_CFG_LogMsgHTTPInfo,
	[TR069_PARAM_LogMsgIGMPInfo] = F_CFG_LogMsgIGMPInfo,
	[TR069_PARAM_LogMsgPkgTotalOneSec] = F_CFG_LogMsgPkgTotalOneSec,
	[TR069_PARAM_LogMsgByteTotalOneSec] = F_CFG_LogMsgByteTotalOneSec,
	[TR069_PARAM_LogMsgPkgLostRate] = F_CFG_LogMsgPkgLostRate,
	[TR069_PARAM_LogMsgAvarageRate] = F_CFG_LogMsgAvarageRate,
	[TR069_PARAM_LogMsgBUFFER] = F_CFG_LogMsgBUFFER,
	[TR069_PARAM_LogMsgERROR] = F_CFG_LogMsgERROR,
	[TR069_PARAM_LogMsgVendorExt] = F_CFG_LogMsgVendorExt,
	[TR069_PARAM_OperatorInfo] = F_CFG_OperatorInfo,
	[TR069_PARAM_IntegrityCheck] = 0,
	[TR069_PARAM_UpgradeURL] = F_CFG_UpgradeDomain,
	[TR069_PARAM_BrowserURL2] = F_CFG_AuthURLBackup,
	[TR069_PARAM_BrowserTagURL] = 0,
	[TR069_PARAM_AdministratorPassword] = F_CFG_AdministratorPassword,
	[TR069_PARAM_CUserPassword] = F_CFG_CUserPassword,
	[TR069_PARAM_UserProvince] = F_CFG_UserProvince,
	[TR069_PARAM_PhyMemSize] = 0,
	[TR069_PARAM_StorageSize] = 0,
	[TR069_PARAM_AlarmReason] = 0,
	[TR069_PARAM_AlarmPacketsLostRate] = 0,
	[TR069_PARAM_AlarmFramesLost] = 0,
	[TR069_PARAM_TvmsgResServer] = F_CFG_TvmsgResServer,
	[TR069_PARAM_TvmsgResInterval] = F_CFG_TvmsgResInterval,
	[TR069_PARAM_SQMEnableEPG] = F_CFG_SQMEnableEPG,
	[TR069_PARAM_SQMEnableMedia] = F_CFG_SQMEnableMedia,
	[TR069_PARAM_SQMEnableMediaBeforeEC] = F_CFG_SQMEnableMediaBeforeEC,
	[TR069_PARAM_SQMEnableWarning] = F_CFG_SQMEnableWarning,
	[TR069_PARAM_SQMEnableTelnet] = F_CFG_SQMEnableTelnet,
	[TR069_PARAM_SQMMediaInterval] = F_CFG_SQMMediaInterval,
	[TR069_PARAM_SQMServerAddress] = F_CFG_SQMServerAddress,
	[TR069_PARAM_SQMServerPort] = F_CFG_SQMServerPort,
	[TR069_PARAM_ConfigFile] = 0,
	[TR069_PARAM_CurrentLanguage] = 0,
	[TR069_PARAM_AvailableLanguages] = 0,
	[TR069_PARAM_IsSupportIPv6] = 0,
	[TR069_PARAM_PacketLostTestEnable] = 0,
	[TR069_PARAM_PacketLostTestUDPPort] = 0,
	[TR069_PARAM_PacketLostTestBand] = 0,
	[TR069_PARAM_AuthServiceInfoPPPOEEnable] = 0,
	[TR069_PARAM_AuthServiceInfoPPPOEID2 ] = 0,
	[TR069_PARAM_AuthServiceInfoPPPOEPassword2] = 0,
	[TR069_PARAM_AuthServiceInfoUserID2] = 0,
	[TR069_PARAM_AuthServiceInfoUserIDPassword2] = 0
};

/* parameter name table orderd by code */
INT8_T* tr069_parameter_name_table[TR069_PARAM_NUM] = { NULL };

static void tr069_parameter_init_name_table()
{
	if (g_tr069_stand == TR069_STAND_CU) {
		tr069_parameter_name_table[TR069_PARAM_Manufacturer] = "Device.DeviceInfo.Manufacturer";
		tr069_parameter_name_table[TR069_PARAM_ManufacturerOUI] = "Device.DeviceInfo.ManufacturerOUI";
		tr069_parameter_name_table[TR069_PARAM_ProductClass] = "Device.DeviceInfo.ProductClass";
		tr069_parameter_name_table[TR069_PARAM_SerialNumber] = "Device.DeviceInfo.SerialNumber";
		tr069_parameter_name_table[TR069_PARAM_HardwareVersion] = "Device.DeviceInfo.HardwareVersion";
		tr069_parameter_name_table[TR069_PARAM_SoftwareVersion] = "Device.DeviceInfo.SoftwareVersion";
		tr069_parameter_name_table[TR069_PARAM_ProvisioningCode] = "Device.DeviceInfo.ProvisioningCode";
		tr069_parameter_name_table[TR069_PARAM_DeviceStatus] = "Device.DeviceInfo.DeviceStatus";
		tr069_parameter_name_table[TR069_PARAM_KernelVersion] = "Device.DeviceInfo.KernelVersion";
		tr069_parameter_name_table[TR069_PARAM_FsVersion] = "Device.DeviceInfo.FsVersion";
		tr069_parameter_name_table[TR069_PARAM_AppVersion] = "Device.DeviceInfo.AppVersion";
		tr069_parameter_name_table[TR069_PARAM_StreamingControlProtocols] = "Device.STBService.StreamingControlProtocols";
		tr069_parameter_name_table[TR069_PARAM_StreamingTransportProtocols] = "Device.STBService.StreamingTransportProtocols";
		tr069_parameter_name_table[TR069_PARAM_StreamingTransportControlProtocols] = "Device.STBService.StreamingTransportControlProtocols";
		tr069_parameter_name_table[TR069_PARAM_DownloadTransportProtocols] = "Device.STBService.DownloadTransportProtocols";
		tr069_parameter_name_table[TR069_PARAM_MultiplexTypes] = "Device.STBService.MultiplexTypes";
		tr069_parameter_name_table[TR069_PARAM_MaxDejitteringBufferSize] = "Device.STBService.MaxDejitteringBufferSize";
		tr069_parameter_name_table[TR069_PARAM_AudioStandards] = "Device.STBService.AudioStandards";
		tr069_parameter_name_table[TR069_PARAM_VideoStandards] = "Device.STBService.VideoStandards";
		tr069_parameter_name_table[TR069_PARAM_ManageSerURL] = "Device.ManagementServer.URL";
		tr069_parameter_name_table[TR069_PARAM_ManageSerUsername] = "Device.ManagementServer.Username";
		tr069_parameter_name_table[TR069_PARAM_ManageSerPassword] = "Device.ManagementServer.Password";
		tr069_parameter_name_table[TR069_PARAM_ConnectionRequestURL] = "Device.ManagementServer.ConnectionRequestURL";
		tr069_parameter_name_table[TR069_PARAM_ConnectionRequestUsername] = "Device.ManagementServer.ConnectionRequestUsername";
		tr069_parameter_name_table[TR069_PARAM_ConnectionRequestPassword] = "Device.ManagementServer.ConnectionRequestPassword";
		tr069_parameter_name_table[TR069_PARAM_PeriodicInformEnable] = "Device.ManagementServer.PeriodicInformEnable";
		tr069_parameter_name_table[TR069_PARAM_PeriodicInformInterval] = "Device.ManagementServer.PeriodicInformInterval";
		tr069_parameter_name_table[TR069_PARAM_PeriodicInformTime] = "Device.ManagementServer.PeriodicInformTime";
		tr069_parameter_name_table[TR069_PARAM_UpgradesManaged] = "Device.ManagementServer.UpgradesManaged";
		tr069_parameter_name_table[TR069_PARAM_ParameterKey] = "Device.ManagementServer.ParameterKey";
		tr069_parameter_name_table[TR069_PARAM_ManageSerURLBackup] = "";
		tr069_parameter_name_table[TR069_PARAM_UDPConnectionRequestAddress] = "Device.ManagementServer.UDPConnectionRequestAddress";
		tr069_parameter_name_table[TR069_PARAM_UDPConnectionRequestNtfLimit] = "Device.ManagementServer.UDPConnectionRequestAddressNotificationLimit";
		tr069_parameter_name_table[TR069_PARAM_STUNEnable] = "Device.ManagementServer.STUNEnable";
		tr069_parameter_name_table[TR069_PARAM_STUNServerAddress] = "Device.ManagementServer.STUNServerAddress";
		tr069_parameter_name_table[TR069_PARAM_STUNServerPort] = "Device.ManagementServer.STUNServerPort";
		tr069_parameter_name_table[TR069_PARAM_STUNUsername] = "Device.ManagementServer.STUNUsername";
		tr069_parameter_name_table[TR069_PARAM_STUNPassword] = "Device.ManagementServer.STUNPassword";
		tr069_parameter_name_table[TR069_PARAM_STUNMaxKeepAlivePeriod] = "Device.ManagementServer.STUNMaximumKeepAlivePeriod";
		tr069_parameter_name_table[TR069_PARAM_STUNMinKeepAlivePeriod] = "Device.ManagementServer.STUNMinimumKeepAlivePeriod";
		tr069_parameter_name_table[TR069_PARAM_NATDetected] = "Device.ManagementServer.NATDetected";
		tr069_parameter_name_table[TR069_PARAM_NTPServer1] = "Device.Time.NTPServer";
		tr069_parameter_name_table[TR069_PARAM_LocalTimeZone] = "Device.Time.LocalTimeZone";
		tr069_parameter_name_table[TR069_PARAM_CurrentLocalTime] = "Device.Time.CurrentLocalTime";
		tr069_parameter_name_table[TR069_PARAM_Device] = "Device.LAN.Device";
		tr069_parameter_name_table[TR069_PARAM_DhcpUser] = "Device.LAN.DhcpUser";
		tr069_parameter_name_table[TR069_PARAM_DhcpPassword] = "Device.LAN.DhcpPassword";
		tr069_parameter_name_table[TR069_PARAM_AddressingType] = "Device.LAN.AddressingType";
		tr069_parameter_name_table[TR069_PARAM_IPAddressReal] = "Device.LAN.IPAddress";
		tr069_parameter_name_table[TR069_PARAM_SubnetMaskReal] = "Device.LAN.SubnetMask";
		tr069_parameter_name_table[TR069_PARAM_DefaultGatewayReal] = "Device.LAN.DefaultGateway";
		tr069_parameter_name_table[TR069_PARAM_DNSServer0Real] = "Device.LAN.DNSServers";
		tr069_parameter_name_table[TR069_PARAM_DNSServer1Real] = "Device.LAN.DNSServers2";
		tr069_parameter_name_table[TR069_PARAM_MACAddress] = "Device.LAN.MACAddress";
		tr069_parameter_name_table[TR069_PARAM_ConnectionUpTime] = "Device.LAN.Stats.ConnectionUpTime";
		tr069_parameter_name_table[TR069_PARAM_TotalBytesSent] = "Device.LAN.Stats.TotalBytesSent";
		tr069_parameter_name_table[TR069_PARAM_TotalBytesReceived] = "Device.LAN.Stats.TotalBytesReceived";
		tr069_parameter_name_table[TR069_PARAM_TotalPacketsSent] = "Device.LAN.Stats.TotalPacketsSent";
		tr069_parameter_name_table[TR069_PARAM_TotalPacketsReceived] = "Device.LAN.Stats.TotalPacketsReceived";
		tr069_parameter_name_table[TR069_PARAM_CurrentDayInterval] = "Device.LAN.Stats.CurrentDayInterval";
		tr069_parameter_name_table[TR069_PARAM_CurrentDayBytesSent] = "Device.LAN.Stats.CurrentDayBytesSent";
		tr069_parameter_name_table[TR069_PARAM_CurrentDayBytesReceived] = "Device.LAN.Stats.CurrentDayBytesReceived";
		tr069_parameter_name_table[TR069_PARAM_CurrentDayPacketsSent] = "Device.LAN.Stats.CurrentDayPacketsSent";
		tr069_parameter_name_table[TR069_PARAM_CurrentDayPacketsReceived] = "Device.LAN.Stats.CurrentDayPacketsReceived";
		tr069_parameter_name_table[TR069_PARAM_QuarterHourInterval] = "Device.LAN.Stats.QuarterHourInterval";
		tr069_parameter_name_table[TR069_PARAM_QuarterHourBytesSent] = "Device.LAN.Stats.QuarterHourBytesSent";
		tr069_parameter_name_table[TR069_PARAM_QuarterHourBytesReceived] = "Device.LAN.Stats.QuarterHourBytesReceived";
		tr069_parameter_name_table[TR069_PARAM_QuarterHourPacketsSent] = "Device.LAN.Stats.QuarterHourPacketsSent";
		tr069_parameter_name_table[TR069_PARAM_QuarterHourPacketsReceived] = "Device.LAN.Stats.QuarterHourPacketsReceived";
		tr069_parameter_name_table[TR069_PARAM_PingDiagState] = "Device.LAN.IPPingDiagnostics.DiagnosticsState";
		tr069_parameter_name_table[TR069_PARAM_PingDiagHost] = "Device.LAN.IPPingDiagnostics.Host";
		tr069_parameter_name_table[TR069_PARAM_PingDiagReptNumber] = "Device.LAN.IPPingDiagnostics.NumberOfRepetitions";
		tr069_parameter_name_table[TR069_PARAM_PingDiagTimeout] = "Device.LAN.IPPingDiagnostics.Timeout";
		tr069_parameter_name_table[TR069_PARAM_PingDiagDataBlockSize] = "Device.LAN.IPPingDiagnostics.DataBlockSize";
		tr069_parameter_name_table[TR069_PARAM_PingDiagDSCP] = "Device.LAN.IPPingDiagnostics.DSCP";
		tr069_parameter_name_table[TR069_PARAM_PingDiagSuccessCount] = "Device.LAN.IPPingDiagnostics.SuccessCount";
		tr069_parameter_name_table[TR069_PARAM_PingDiagFailureCount] = "Device.LAN.IPPingDiagnostics.FailureCount";
		tr069_parameter_name_table[TR069_PARAM_PingDiagAvgRespTime] = "Device.LAN.IPPingDiagnostics.AverageResponseTime";
		tr069_parameter_name_table[TR069_PARAM_PingDiagMinRespTime] = "Device.LAN.IPPingDiagnostics.MinimumResponseTime";
		tr069_parameter_name_table[TR069_PARAM_PingDiagMaxRespTime] = "Device.LAN.IPPingDiagnostics.MaximumResponseTime";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagState] = "Device.LAN.TraceRouteDiagnostics.DiagnosticsState";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagHost] = "Device.LAN.TraceRouteDiagnostics.Host";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagTimeout] = "Device.LAN.TraceRouteDiagnostics.Timeout";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagDataBlockSize] = "Device.LAN.TraceRouteDiagnostics.DataBlockSize";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagMaxHopCount] = "Device.LAN.TraceRouteDiagnostics.MaxHopCount";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagDSCP] = "Device.LAN.TraceRouteDiagnostics.DSCP";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagRespTime] = "Device.LAN.TraceRouteDiagnostics.ResponseTime";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagHopsNumber] = "Device.LAN.TraceRouteDiagnostics.NumberOfRouteHops";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST1] = "Device.LAN.TraceRouteDiagnostics.RouteHops.1.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST2] = "Device.LAN.TraceRouteDiagnostics.RouteHops.2.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST3] = "Device.LAN.TraceRouteDiagnostics.RouteHops.3.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST4] = "Device.LAN.TraceRouteDiagnostics.RouteHops.4.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST5] = "Device.LAN.TraceRouteDiagnostics.RouteHops.5.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST6] = "Device.LAN.TraceRouteDiagnostics.RouteHops.6.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST7] = "Device.LAN.TraceRouteDiagnostics.RouteHops.7.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST8] = "Device.LAN.TraceRouteDiagnostics.RouteHops.8.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST9] = "Device.LAN.TraceRouteDiagnostics.RouteHops.9.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST10] = "Device.LAN.TraceRouteDiagnostics.RouteHops.10.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST11] = "Device.LAN.TraceRouteDiagnostics.RouteHops.11.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST12] = "Device.LAN.TraceRouteDiagnostics.RouteHops.12.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST13] = "Device.LAN.TraceRouteDiagnostics.RouteHops.13.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST14] = "Device.LAN.TraceRouteDiagnostics.RouteHops.14.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST15] = "Device.LAN.TraceRouteDiagnostics.RouteHops.15.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST16] = "Device.LAN.TraceRouteDiagnostics.RouteHops.16.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST17] = "Device.LAN.TraceRouteDiagnostics.RouteHops.17.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST18] = "Device.LAN.TraceRouteDiagnostics.RouteHops.18.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST19] = "Device.LAN.TraceRouteDiagnostics.RouteHops.19.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST20] = "Device.LAN.TraceRouteDiagnostics.RouteHops.20.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST21] = "Device.LAN.TraceRouteDiagnostics.RouteHops.21.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST22] = "Device.LAN.TraceRouteDiagnostics.RouteHops.22.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST23] = "Device.LAN.TraceRouteDiagnostics.RouteHops.23.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST24] = "Device.LAN.TraceRouteDiagnostics.RouteHops.24.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST25] = "Device.LAN.TraceRouteDiagnostics.RouteHops.25.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST26] = "Device.LAN.TraceRouteDiagnostics.RouteHops.26.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST27] = "Device.LAN.TraceRouteDiagnostics.RouteHops.27.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST28] = "Device.LAN.TraceRouteDiagnostics.RouteHops.28.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST29] = "Device.LAN.TraceRouteDiagnostics.RouteHops.29.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST30] = "Device.LAN.TraceRouteDiagnostics.RouteHops.30.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST31] = "Device.LAN.TraceRouteDiagnostics.RouteHops.31.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST32] = "Device.LAN.TraceRouteDiagnostics.RouteHops.32.HopHost";
		tr069_parameter_name_table[TR069_PARAM_STBID] = "Device.X_CU_STB.STBInfo.STBID";
		tr069_parameter_name_table[TR069_PARAM_PPPoEID] = "Device.X_CU_STB.AuthServiceInfo.PPPOEID";
		tr069_parameter_name_table[TR069_PARAM_PPPoEPassword] = "Device.X_CU_STB.AuthServiceInfo.PPPOEPassword";
		tr069_parameter_name_table[TR069_PARAM_UserID] = "Device.X_CU_STB.AuthServiceInfo.UserID";
		tr069_parameter_name_table[TR069_PARAM_UserPassword] = "Device.X_CU_STB.AuthServiceInfo.UserIDPassword";
		tr069_parameter_name_table[TR069_PARAM_AuthURL] = "Device.X_CU_STB.STBInfo.BrowserURL1";
		tr069_parameter_name_table[TR069_PARAM_LogServerUrl] = "Device.X_CU_STB.StatisticConfiguration.LogServerUrl";
		tr069_parameter_name_table[TR069_PARAM_LogUploadInterval] = "Device.X_CU_STB.StatisticConfiguration.LogUploadInterval";
		tr069_parameter_name_table[TR069_PARAM_LogRecordInterval] = "Device.X_CU_STB.StatisticConfiguration.LogRecordInterval";
		tr069_parameter_name_table[TR069_PARAM_StatInterval] = "Device.X_CU_STB.StatisticConfiguration.StatInterval";
		tr069_parameter_name_table[TR069_PARAM_PacketsLostR1] = "Device.X_CU_STB.StatisticConfiguration.PacketsLostR1";
		tr069_parameter_name_table[TR069_PARAM_PacketsLostR2] = "Device.X_CU_STB.StatisticConfiguration.PacketsLostR2";
		tr069_parameter_name_table[TR069_PARAM_PacketsLostR3] = "Device.X_CU_STB.StatisticConfiguration.PacketsLostR3";
		tr069_parameter_name_table[TR069_PARAM_PacketsLostR4] = "Device.X_CU_STB.StatisticConfiguration.PacketsLostR4";
		tr069_parameter_name_table[TR069_PARAM_PacketsLostR5] = "Device.X_CU_STB.StatisticConfiguration.PacketsLostR5";
		tr069_parameter_name_table[TR069_PARAM_BitRateR1] = "Device.X_CU_STB.StatisticConfiguration.BitRateR1";
		tr069_parameter_name_table[TR069_PARAM_BitRateR2] = "Device.X_CU_STB.StatisticConfiguration.BitRateR2";
		tr069_parameter_name_table[TR069_PARAM_BitRateR3] = "Device.X_CU_STB.StatisticConfiguration.BitRateR3";
		tr069_parameter_name_table[TR069_PARAM_BitRateR4] = "Device.X_CU_STB.StatisticConfiguration.BitRateR4";
		tr069_parameter_name_table[TR069_PARAM_BitRateR5] = "Device.X_CU_STB.StatisticConfiguration.BitRateR5";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR1] = "Device.X_CU_STB.StatisticConfiguration.FramesLostR1";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR2] = "Device.X_CU_STB.StatisticConfiguration.FramesLostR2";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR3] = "Device.X_CU_STB.StatisticConfiguration.FramesLostR3";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR4] = "Device.X_CU_STB.StatisticConfiguration.FramesLostR4";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR5] = "Device.X_CU_STB.StatisticConfiguration.FramesLostR5";
		tr069_parameter_name_table[TR069_PARAM_Startpoint] = "Device.X_CU_STB.ServiceStatistics.Startpoint";
		tr069_parameter_name_table[TR069_PARAM_Endpoint] = "Device.X_CU_STB.ServiceStatistics.Endpoint";
		tr069_parameter_name_table[TR069_PARAM_AuthNumbers] = "Device.X_CU_STB.ServiceStatistics.AuthNumbers";
		tr069_parameter_name_table[TR069_PARAM_AuthFailNumbers] = "Device.X_CU_STB.ServiceStatistics.AuthFailNumbers";
		tr069_parameter_name_table[TR069_PARAM_AuthFailInfo] = "Device.X_CU_STB.ServiceStatistics.AuthFailInfo";
		tr069_parameter_name_table[TR069_PARAM_MultiReqNumbers] = "Device.X_CU_STB.ServiceStatistics.MultiReqNumbers";
		tr069_parameter_name_table[TR069_PARAM_MultiFailNumbers] = "Device.X_CU_STB.ServiceStatistics.MultiFailNumbers";
		tr069_parameter_name_table[TR069_PARAM_MultiFailInfo] = "Device.X_CU_STB.ServiceStatistics.MultiFailInfo";
		tr069_parameter_name_table[TR069_PARAM_VodReqNumbers] = "Device.X_CU_STB.ServiceStatistics.VodReqNumbers";
		tr069_parameter_name_table[TR069_PARAM_VodFailNumbers] = "Device.X_CU_STB.ServiceStatistics.VodFailNumbers";
		tr069_parameter_name_table[TR069_PARAM_VodFailInfo] = "Device.X_CU_STB.ServiceStatistics.VodFailInfo";
		tr069_parameter_name_table[TR069_PARAM_HTTPReqNumbers] = "Device.X_CU_STB.ServiceStatistics.HTTPReqNumbers";
		tr069_parameter_name_table[TR069_PARAM_HTTPFailNumbers] = "Device.X_CU_STB.ServiceStatistics.HTTPFailNumbers";
		tr069_parameter_name_table[TR069_PARAM_HTTPFailInfo] = "Device.X_CU_STB.ServiceStatistics.HTTPFailInfo";
		tr069_parameter_name_table[TR069_PARAM_MutiAbendNumbers] = "Device.X_CU_STB.ServiceStatistics.MutiAbendNumbers";
		tr069_parameter_name_table[TR069_PARAM_VODAbendNumbers] = "Device.X_CU_STB.ServiceStatistics.VODAbendNumbers";
		tr069_parameter_name_table[TR069_PARAM_PlayErrorNumbers] = "Device.X_CU_STB.ServiceStatistics.PlayErrorNumbers";
		tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR1Nmb] = "Device.X_CU_STB.ServiceStatistics.MultiPacketsLostR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR2Nmb] = "Device.X_CU_STB.ServiceStatistics.MultiPacketsLostR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR3Nmb] = "Device.X_CU_STB.ServiceStatistics.MultiPacketsLostR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR4Nmb] = "Device.X_CU_STB.ServiceStatistics.MultiPacketsLostR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR5Nmb] = "Device.X_CU_STB.ServiceStatistics.MultiPacketsLostR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR1Nmb] = "Device.X_CU_STB.ServiceStatistics.VODPacketsLostR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR2Nmb] = "Device.X_CU_STB.ServiceStatistics.VODPacketsLostR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR3Nmb] = "Device.X_CU_STB.ServiceStatistics.VODPacketsLostR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR4Nmb] = "Device.X_CU_STB.ServiceStatistics.VODPacketsLostR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR5Nmb] = "Device.X_CU_STB.ServiceStatistics.VODPacketsLostR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiBitRateR1Nmb] = "Device.X_CU_STB.ServiceStatistics.MultiBitRateR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiBitRateR2Nmb] = "Device.X_CU_STB.ServiceStatistics.MultiBitRateR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiBitRateR3Nmb] = "Device.X_CU_STB.ServiceStatistics.MultiBitRateR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiBitRateR4Nmb] = "Device.X_CU_STB.ServiceStatistics.MultiBitRateR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiBitRateR5Nmb] = "Device.X_CU_STB.ServiceStatistics.MultiBitRateR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODBitRateR1Nmb] = "Device.X_CU_STB.ServiceStatistics.VODBitRateR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODBitRateR2Nmb] = "Device.X_CU_STB.ServiceStatistics.VODBitRateR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODBitRateR3Nmb] = "Device.X_CU_STB.ServiceStatistics.VODBitRateR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODBitRateR4Nmb] = "Device.X_CU_STB.ServiceStatistics.VODBitRateR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODBitRateR5Nmb] = "Device.X_CU_STB.ServiceStatistics.VODBitRateR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR1Nmb] = "Device.X_CU_STB.ServiceStatistics.FramesLostR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR2Nmb] = "Device.X_CU_STB.ServiceStatistics.FramesLostR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR3Nmb] = "Device.X_CU_STB.ServiceStatistics.FramesLostR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR4Nmb] = "Device.X_CU_STB.ServiceStatistics.FramesLostR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR5Nmb] = "Device.X_CU_STB.ServiceStatistics.FramesLostR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_PlayErrorInfo] = "Device.X_CU_STB.ServiceStatistics.PlayErrorInfo";
		tr069_parameter_name_table[TR069_PARAM_LogMsgEnable] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgOrFile] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgFtpServer] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgFtpUser] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgFtpPassword] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgDuration] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgRTSPInfo] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgHTTPInfo] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgIGMPInfo] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgPkgTotalOneSec] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgByteTotalOneSec] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgPkgLostRate] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgAvarageRate] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgBUFFER] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgERROR] = "";
		tr069_parameter_name_table[TR069_PARAM_LogMsgVendorExt] = "";
		tr069_parameter_name_table[TR069_PARAM_OperatorInfo] = "Device.X_CU_STB.STBInfo.OperatorInfo";
		tr069_parameter_name_table[TR069_PARAM_IntegrityCheck] = "Device.X_CU_STB.STBInfo.IntegrityCheck";
		tr069_parameter_name_table[TR069_PARAM_UpgradeURL] = "Device.X_CU_STB.STBInfo.UpgradeURL";
		tr069_parameter_name_table[TR069_PARAM_BrowserURL2] = "Device.X_CU_STB.STBInfo.BrowserURL2";
		tr069_parameter_name_table[TR069_PARAM_BrowserTagURL] = "Device.X_CU_STB.STBInfo.BrowserTagURL";
		tr069_parameter_name_table[TR069_PARAM_AdministratorPassword] = "Device.X_CU_STB.STBInfo.AdministratorPassword";
		tr069_parameter_name_table[TR069_PARAM_CUserPassword] = "Device.X_CU_STB.STBInfo.UserPassword";
		tr069_parameter_name_table[TR069_PARAM_UserProvince] = "Device.X_CU_STB.STBInfo.UserProvince";
		tr069_parameter_name_table[TR069_PARAM_PhyMemSize] = "Device.X_CU_STB.STBInfo.PhyMemSize";
		tr069_parameter_name_table[TR069_PARAM_StorageSize] = "Device.X_CU_STB.STBInfo.StorageSize";
		tr069_parameter_name_table[TR069_PARAM_AlarmReason] = "Device.X_CU_STB.Alarm.AlarmReason";
		tr069_parameter_name_table[TR069_PARAM_AlarmPacketsLostRate] = "Device.X_CU_STB.Alarm.PacketsLostRate";
		tr069_parameter_name_table[TR069_PARAM_AlarmFramesLost] = "Device.X_CU_STB.Alarm.FramesLost";
		tr069_parameter_name_table[TR069_PARAM_TvmsgResServer] = "Device.X_FV.TVMS_IP";
		tr069_parameter_name_table[TR069_PARAM_TvmsgResInterval] = "Device.X_FV.RES_POLL_INTERVAL";
		tr069_parameter_name_table[TR069_PARAM_SQMEnableEPG] = "Device.X_FV.SQM.EnableEPG";
		tr069_parameter_name_table[TR069_PARAM_SQMEnableMedia] = "Device.X_FV.SQM.EnableMedia";
		tr069_parameter_name_table[TR069_PARAM_SQMEnableMediaBeforeEC] = "Device.X_FV.SQM.EnableMediaBeforeEC";
		tr069_parameter_name_table[TR069_PARAM_SQMEnableWarning] = "Device.X_FV.SQM.EnableWarning";
		tr069_parameter_name_table[TR069_PARAM_SQMEnableTelnet] = "Device.X_FV.SQM.EnableTelnet";
		tr069_parameter_name_table[TR069_PARAM_SQMMediaInterval] = "Device.X_FV.SQM.MediaInterval";
		tr069_parameter_name_table[TR069_PARAM_SQMServerAddress] = "Device.X_FV.SQM.ServerAddress";
		tr069_parameter_name_table[TR069_PARAM_SQMServerPort] = "Device.X_FV.SQM.ServerPort";
		tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoPPPOEEnable] = "Device.X_CU_STB.AuthServiceInfo.PPPOEEnable";
		tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoPPPOEID2 ] = "Device.X_CU_STB.AuthServiceInfo.PPPOEID2";
		tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoPPPOEPassword2] = "Device.X_CU_STB.AuthServiceInfo.PPPOEPassword2";
		tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoUserID2] = "Device.X_CU_STB.AuthServiceInfo.UserID2";
		tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoUserIDPassword2] = "Device.X_CU_STB.AuthServiceInfo.UserIDPassword2";
	} else {
		tr069_parameter_name_table[TR069_PARAM_Manufacturer] = "Device.DeviceInfo.Manufacturer";
		tr069_parameter_name_table[TR069_PARAM_ManufacturerOUI] = "Device.DeviceInfo.ManufacturerOUI";
		tr069_parameter_name_table[TR069_PARAM_ProductClass] = "Device.DeviceInfo.ProductClass";
		tr069_parameter_name_table[TR069_PARAM_SerialNumber] = "Device.DeviceInfo.SerialNumber";
		tr069_parameter_name_table[TR069_PARAM_HardwareVersion] = "Device.DeviceInfo.HardwareVersion";
		tr069_parameter_name_table[TR069_PARAM_SoftwareVersion] = "Device.DeviceInfo.SoftwareVersion";
		tr069_parameter_name_table[TR069_PARAM_ProvisioningCode] = "Device.DeviceInfo.ProvisioningCode";
		tr069_parameter_name_table[TR069_PARAM_DeviceStatus] = "Device.DeviceInfo.DeviceStatus";
		tr069_parameter_name_table[TR069_PARAM_KernelVersion] = "Device.DeviceInfo.KernelVersion";
		tr069_parameter_name_table[TR069_PARAM_FsVersion] = "Device.DeviceInfo.FsVersion";
		tr069_parameter_name_table[TR069_PARAM_AppVersion] = "Device.DeviceInfo.AppVersion";
		tr069_parameter_name_table[TR069_PARAM_ModelName] = "Device.DeviceInfo.ModelName";
		tr069_parameter_name_table[TR069_PARAM_ModelID] = "Device.DeviceInfo.ModelID";
		tr069_parameter_name_table[TR069_PARAM_Description] = "Device.DeviceInfo.Description";
		tr069_parameter_name_table[TR069_PARAM_ConfigFileVersion] = "Device.DeviceInfo.ConfigFileVersion";
		tr069_parameter_name_table[TR069_PARAM_UpTime] = "Device.DeviceInfo.UpTime";
		tr069_parameter_name_table[TR069_PARAM_FirstUseDate] = "Device.DeviceInfo.FirstUseDate";
		tr069_parameter_name_table[TR069_PARAM_DeviceLog] = "Device.DeviceInfo.DeviceLog";
		tr069_parameter_name_table[TR069_PARAM_StreamingControlProtocols] = "Device.STBService.StreamingControlProtocols";
		tr069_parameter_name_table[TR069_PARAM_StreamingTransportProtocols] = "Device.STBService.StreamingTransportProtocols";
		tr069_parameter_name_table[TR069_PARAM_StreamingTransportControlProtocols] = "Device.STBService.StreamingTransportControlProtocols";
		tr069_parameter_name_table[TR069_PARAM_DownloadTransportProtocols] = "Device.STBService.DownloadTransportProtocols";
		tr069_parameter_name_table[TR069_PARAM_MultiplexTypes] = "Device.STBService.MultiplexTypes";
		tr069_parameter_name_table[TR069_PARAM_MaxDejitteringBufferSize] = "Device.STBService.MaxDejitteringBufferSize";
		tr069_parameter_name_table[TR069_PARAM_AudioStandards] = "Device.STBService.AudioStandards";
		tr069_parameter_name_table[TR069_PARAM_VideoStandards] = "Device.STBService.VideoStandards";
		tr069_parameter_name_table[TR069_PARAM_ManageSerURL] = "Device.ManagementServer.URL";
		tr069_parameter_name_table[TR069_PARAM_ManageSerUsername] = "Device.ManagementServer.Username";
		tr069_parameter_name_table[TR069_PARAM_ManageSerPassword] = "Device.ManagementServer.Password";
		tr069_parameter_name_table[TR069_PARAM_ConnectionRequestURL] = "Device.ManagementServer.ConnectionRequestURL";
		tr069_parameter_name_table[TR069_PARAM_ConnectionRequestUsername] = "Device.ManagementServer.ConnectionRequestUsername";
		tr069_parameter_name_table[TR069_PARAM_ConnectionRequestPassword] = "Device.ManagementServer.ConnectionRequestPassword";
		tr069_parameter_name_table[TR069_PARAM_PeriodicInformEnable] = "Device.ManagementServer.PeriodicInformEnable";
		tr069_parameter_name_table[TR069_PARAM_PeriodicInformInterval] = "Device.ManagementServer.PeriodicInformInterval";
		tr069_parameter_name_table[TR069_PARAM_PeriodicInformTime] = "Device.ManagementServer.PeriodicInformTime";
		tr069_parameter_name_table[TR069_PARAM_UpgradesManaged] = "Device.ManagementServer.UpgradesManaged";
		tr069_parameter_name_table[TR069_PARAM_ParameterKey] = "Device.ManagementServer.ParameterKey";
		tr069_parameter_name_table[TR069_PARAM_ManageSerURLBackup] = "Device.ManagementServer.URLBackup";
		tr069_parameter_name_table[TR069_PARAM_UDPConnectionRequestAddress] = "Device.ManagementServer.UDPConnectionRequestAddress";
		tr069_parameter_name_table[TR069_PARAM_UDPConnectionRequestNtfLimit] = "Device.ManagementServer.UDPConnectionRequestAddressNotificationLimit";
		tr069_parameter_name_table[TR069_PARAM_STUNEnable] = "Device.ManagementServer.STUNEnable";
		tr069_parameter_name_table[TR069_PARAM_STUNServerAddress] = "Device.ManagementServer.STUNServerAddress";
		tr069_parameter_name_table[TR069_PARAM_STUNServerPort] = "Device.ManagementServer.STUNServerPort";
		tr069_parameter_name_table[TR069_PARAM_STUNUsername] = "Device.ManagementServer.STUNUsername";
		tr069_parameter_name_table[TR069_PARAM_STUNPassword] = "Device.ManagementServer.STUNPassword";
		tr069_parameter_name_table[TR069_PARAM_STUNMaxKeepAlivePeriod] = "Device.ManagementServer.STUNMaximumKeepAlivePeriod";
		tr069_parameter_name_table[TR069_PARAM_STUNMinKeepAlivePeriod] = "Device.ManagementServer.STUNMinimumKeepAlivePeriod";
		tr069_parameter_name_table[TR069_PARAM_NATDetected] = "Device.ManagementServer.NATDetected";
		tr069_parameter_name_table[TR069_PARAM_NTPServer1] = "Device.Time.NTPServer1";
		tr069_parameter_name_table[TR069_PARAM_LocalTimeZone] = "Device.Time.LocalTimeZone";
		tr069_parameter_name_table[TR069_PARAM_CurrentLocalTime] = "Device.Time.CurrentLocalTime";
		tr069_parameter_name_table[TR069_PARAM_Device] = "Device.LAN.Device";
		tr069_parameter_name_table[TR069_PARAM_DhcpUser] = "Device.LAN.DhcpUser";
		tr069_parameter_name_table[TR069_PARAM_DhcpPassword] = "Device.LAN.DhcpPassword";
		tr069_parameter_name_table[TR069_PARAM_AddressingType] = "Device.LAN.AddressingType";
		tr069_parameter_name_table[TR069_PARAM_IPAddressReal] = "Device.LAN.IPAddress";
		tr069_parameter_name_table[TR069_PARAM_SubnetMaskReal] = "Device.LAN.SubnetMask";
		tr069_parameter_name_table[TR069_PARAM_DefaultGatewayReal] = "Device.LAN.DefaultGateway";
		tr069_parameter_name_table[TR069_PARAM_DNSServer0Real] = "Device.LAN.DNSServer0";
		tr069_parameter_name_table[TR069_PARAM_DNSServer1Real] = "Device.LAN.DNSServer1";
		tr069_parameter_name_table[TR069_PARAM_MACAddress] = "Device.LAN.MACAddress";
		tr069_parameter_name_table[TR069_PARAM_ConnectionUpTime] = "Device.LAN.Stats.ConnectionUpTime";
		tr069_parameter_name_table[TR069_PARAM_TotalBytesSent] = "Device.LAN.Stats.TotalBytesSent";
		tr069_parameter_name_table[TR069_PARAM_TotalBytesReceived] = "Device.LAN.Stats.TotalBytesReceived";
		tr069_parameter_name_table[TR069_PARAM_TotalPacketsSent] = "Device.LAN.Stats.TotalPacketsSent";
		tr069_parameter_name_table[TR069_PARAM_TotalPacketsReceived] = "Device.LAN.Stats.TotalPacketsReceived";
		tr069_parameter_name_table[TR069_PARAM_CurrentDayInterval] = "Device.LAN.Stats.CurrentDayInterval";
		tr069_parameter_name_table[TR069_PARAM_CurrentDayBytesSent] = "Device.LAN.Stats.CurrentDayBytesSent";
		tr069_parameter_name_table[TR069_PARAM_CurrentDayBytesReceived] = "Device.LAN.Stats.CurrentDayBytesReceived";
		tr069_parameter_name_table[TR069_PARAM_CurrentDayPacketsSent] = "Device.LAN.Stats.CurrentDayPacketsSent";
		tr069_parameter_name_table[TR069_PARAM_CurrentDayPacketsReceived] = "Device.LAN.Stats.CurrentDayPacketsReceived";
		tr069_parameter_name_table[TR069_PARAM_QuarterHourInterval] = "Device.LAN.Stats.QuarterHourInterval";
		tr069_parameter_name_table[TR069_PARAM_QuarterHourBytesSent] = "Device.LAN.Stats.QuarterHourBytesSent";
		tr069_parameter_name_table[TR069_PARAM_QuarterHourBytesReceived] = "Device.LAN.Stats.QuarterHourBytesReceived";
		tr069_parameter_name_table[TR069_PARAM_QuarterHourPacketsSent] = "Device.LAN.Stats.QuarterHourPacketsSent";
		tr069_parameter_name_table[TR069_PARAM_QuarterHourPacketsReceived] = "Device.LAN.Stats.QuarterHourPacketsReceived";
		tr069_parameter_name_table[TR069_PARAM_PingDiagState] = "Device.LAN.IPPingDiagnostics.DiagnosticsState";
		tr069_parameter_name_table[TR069_PARAM_PingDiagHost] = "Device.LAN.IPPingDiagnostics.Host";
		tr069_parameter_name_table[TR069_PARAM_PingDiagReptNumber] = "Device.LAN.IPPingDiagnostics.NumberOfRepetitions";
		tr069_parameter_name_table[TR069_PARAM_PingDiagTimeout] = "Device.LAN.IPPingDiagnostics.Timeout";
		tr069_parameter_name_table[TR069_PARAM_PingDiagDataBlockSize] = "Device.LAN.IPPingDiagnostics.DataBlockSize";
		tr069_parameter_name_table[TR069_PARAM_PingDiagDSCP] = "Device.LAN.IPPingDiagnostics.DSCP";
		tr069_parameter_name_table[TR069_PARAM_PingDiagSuccessCount] = "Device.LAN.IPPingDiagnostics.SuccessCount";
		tr069_parameter_name_table[TR069_PARAM_PingDiagFailureCount] = "Device.LAN.IPPingDiagnostics.FailureCount";
		tr069_parameter_name_table[TR069_PARAM_PingDiagAvgRespTime] = "Device.LAN.IPPingDiagnostics.AverageResponseTime";
		tr069_parameter_name_table[TR069_PARAM_PingDiagMinRespTime] = "Device.LAN.IPPingDiagnostics.MinimumResponseTime";
		tr069_parameter_name_table[TR069_PARAM_PingDiagMaxRespTime] = "Device.LAN.IPPingDiagnostics.MaximumResponseTime";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagState] = "Device.LAN.TraceRouteDiagnostics.DiagnosticsState";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagHost] = "Device.LAN.TraceRouteDiagnostics.Host";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagTimeout] = "Device.LAN.TraceRouteDiagnostics.Timeout";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagDataBlockSize] = "Device.LAN.TraceRouteDiagnostics.DataBlockSize";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagMaxHopCount] = "Device.LAN.TraceRouteDiagnostics.MaxHopCount";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagDSCP] = "Device.LAN.TraceRouteDiagnostics.DSCP";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagRespTime] = "Device.LAN.TraceRouteDiagnostics.ResponseTime";
		tr069_parameter_name_table[TR069_PARAM_TracertDiagHopsNumber] = "Device.LAN.TraceRouteDiagnostics.NumberOfRouteHops";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST1] = "Device.LAN.TraceRouteDiagnostics.RouteHops.1.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST2] = "Device.LAN.TraceRouteDiagnostics.RouteHops.2.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST3] = "Device.LAN.TraceRouteDiagnostics.RouteHops.3.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST4] = "Device.LAN.TraceRouteDiagnostics.RouteHops.4.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST5] = "Device.LAN.TraceRouteDiagnostics.RouteHops.5.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST6] = "Device.LAN.TraceRouteDiagnostics.RouteHops.6.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST7] = "Device.LAN.TraceRouteDiagnostics.RouteHops.7.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST8] = "Device.LAN.TraceRouteDiagnostics.RouteHops.8.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST9] = "Device.LAN.TraceRouteDiagnostics.RouteHops.9.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST10] = "Device.LAN.TraceRouteDiagnostics.RouteHops.10.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST11] = "Device.LAN.TraceRouteDiagnostics.RouteHops.11.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST12] = "Device.LAN.TraceRouteDiagnostics.RouteHops.12.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST13] = "Device.LAN.TraceRouteDiagnostics.RouteHops.13.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST14] = "Device.LAN.TraceRouteDiagnostics.RouteHops.14.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST15] = "Device.LAN.TraceRouteDiagnostics.RouteHops.15.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST16] = "Device.LAN.TraceRouteDiagnostics.RouteHops.16.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST17] = "Device.LAN.TraceRouteDiagnostics.RouteHops.17.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST18] = "Device.LAN.TraceRouteDiagnostics.RouteHops.18.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST19] = "Device.LAN.TraceRouteDiagnostics.RouteHops.19.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST20] = "Device.LAN.TraceRouteDiagnostics.RouteHops.20.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST21] = "Device.LAN.TraceRouteDiagnostics.RouteHops.21.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST22] = "Device.LAN.TraceRouteDiagnostics.RouteHops.22.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST23] = "Device.LAN.TraceRouteDiagnostics.RouteHops.23.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST24] = "Device.LAN.TraceRouteDiagnostics.RouteHops.24.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST25] = "Device.LAN.TraceRouteDiagnostics.RouteHops.25.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST26] = "Device.LAN.TraceRouteDiagnostics.RouteHops.26.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST27] = "Device.LAN.TraceRouteDiagnostics.RouteHops.27.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST28] = "Device.LAN.TraceRouteDiagnostics.RouteHops.28.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST29] = "Device.LAN.TraceRouteDiagnostics.RouteHops.29.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST30] = "Device.LAN.TraceRouteDiagnostics.RouteHops.30.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST31] = "Device.LAN.TraceRouteDiagnostics.RouteHops.31.HopHost";
		tr069_parameter_name_table[TR069_PARAM_HOPHOST32] = "Device.LAN.TraceRouteDiagnostics.RouteHops.32.HopHost";
		tr069_parameter_name_table[TR069_PARAM_STBID] = "Device.X_CTC_IPTV.STBID";
		tr069_parameter_name_table[TR069_PARAM_PPPoEID] = "Device.X_CTC_IPTV.ServiceInfo.PPPoEID";
		tr069_parameter_name_table[TR069_PARAM_PPPoEPassword] = "Device.X_CTC_IPTV.ServiceInfo.PPPoEPassword";
		tr069_parameter_name_table[TR069_PARAM_UserID] = "Device.X_CTC_IPTV.ServiceInfo.UserID";
		tr069_parameter_name_table[TR069_PARAM_UserPassword] = "Device.X_CTC_IPTV.ServiceInfo.Password";
		tr069_parameter_name_table[TR069_PARAM_AuthURL] = "Device.X_CTC_IPTV.ServiceInfo.AuthURL";
		tr069_parameter_name_table[TR069_PARAM_AuthURLBackup] = "Device.X_CTC_IPTV.ServiceInfo.AuthURLBackup";
		tr069_parameter_name_table[TR069_PARAM_LogServerUrl] = "Device.X_CTC_IPTV.StatisticConfiguration.LogServerUrl";
		tr069_parameter_name_table[TR069_PARAM_LogUploadInterval] = "Device.X_CTC_IPTV.StatisticConfiguration.LogUploadInterval";
		tr069_parameter_name_table[TR069_PARAM_LogRecordInterval] = "Device.X_CTC_IPTV.StatisticConfiguration.LogRecordInterval";
		tr069_parameter_name_table[TR069_PARAM_StatInterval] = "Device.X_CTC_IPTV.StatisticConfiguration.StatInterval";
		tr069_parameter_name_table[TR069_PARAM_PacketsLostR1] = "Device.X_CTC_IPTV.StatisticConfiguration.PacketsLostR1";
		tr069_parameter_name_table[TR069_PARAM_PacketsLostR2] = "Device.X_CTC_IPTV.StatisticConfiguration.PacketsLostR2";
		tr069_parameter_name_table[TR069_PARAM_PacketsLostR3] = "Device.X_CTC_IPTV.StatisticConfiguration.PacketsLostR3";
		tr069_parameter_name_table[TR069_PARAM_PacketsLostR4] = "Device.X_CTC_IPTV.StatisticConfiguration.PacketsLostR4";
		tr069_parameter_name_table[TR069_PARAM_PacketsLostR5] = "Device.X_CTC_IPTV.StatisticConfiguration.PacketsLostR5";
		tr069_parameter_name_table[TR069_PARAM_BitRateR1] = "Device.X_CTC_IPTV.StatisticConfiguration.BitRateR1";
		tr069_parameter_name_table[TR069_PARAM_BitRateR2] = "Device.X_CTC_IPTV.StatisticConfiguration.BitRateR2";
		tr069_parameter_name_table[TR069_PARAM_BitRateR3] = "Device.X_CTC_IPTV.StatisticConfiguration.BitRateR3";
		tr069_parameter_name_table[TR069_PARAM_BitRateR4] = "Device.X_CTC_IPTV.StatisticConfiguration.BitRateR4";
		tr069_parameter_name_table[TR069_PARAM_BitRateR5] = "Device.X_CTC_IPTV.StatisticConfiguration.BitRateR5";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR1] = "Device.X_CTC_IPTV.StatisticConfiguration.FramesLostR1";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR2] = "Device.X_CTC_IPTV.StatisticConfiguration.FramesLostR2";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR3] = "Device.X_CTC_IPTV.StatisticConfiguration.FramesLostR3";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR4] = "Device.X_CTC_IPTV.StatisticConfiguration.FramesLostR4";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR5] = "Device.X_CTC_IPTV.StatisticConfiguration.FramesLostR5";
		tr069_parameter_name_table[TR069_PARAM_Startpoint] = "Device.X_CTC_IPTV.ServiceStatistics.Startpoint";
		tr069_parameter_name_table[TR069_PARAM_Endpoint] = "Device.X_CTC_IPTV.ServiceStatistics.Endpoint";
		tr069_parameter_name_table[TR069_PARAM_AuthNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.AuthNumbers";
		tr069_parameter_name_table[TR069_PARAM_AuthFailNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.AuthFailNumbers";
		tr069_parameter_name_table[TR069_PARAM_AuthFailInfo] = "Device.X_CTC_IPTV.ServiceStatistics.AuthFailInfo";
		tr069_parameter_name_table[TR069_PARAM_MultiReqNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.MultiReqNumbers";
		tr069_parameter_name_table[TR069_PARAM_MultiFailNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.MultiFailNumbers";
		tr069_parameter_name_table[TR069_PARAM_MultiFailInfo] = "Device.X_CTC_IPTV.ServiceStatistics.MultiFailInfo";
		tr069_parameter_name_table[TR069_PARAM_VodReqNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.VodReqNumbers";
		tr069_parameter_name_table[TR069_PARAM_VodFailNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.VodFailNumbers";
		tr069_parameter_name_table[TR069_PARAM_VodFailInfo] = "Device.X_CTC_IPTV.ServiceStatistics.VodFailInfo";
		tr069_parameter_name_table[TR069_PARAM_HTTPReqNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.HTTPReqNumbers";
		tr069_parameter_name_table[TR069_PARAM_HTTPFailNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.HTTPFailNumbers";
		tr069_parameter_name_table[TR069_PARAM_HTTPFailInfo] = "Device.X_CTC_IPTV.ServiceStatistics.HTTPFailInfo";
		tr069_parameter_name_table[TR069_PARAM_MutiAbendNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.MutiAbendNumbers";
		tr069_parameter_name_table[TR069_PARAM_VODAbendNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.VODAbendNumbers";
		tr069_parameter_name_table[TR069_PARAM_PlayErrorNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.PlayErrorNumbers";
		tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR1Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.MultiPacketsLostR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR2Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.MultiPacketsLostR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR3Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.MultiPacketsLostR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR4Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.MultiPacketsLostR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR5Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.MultiPacketsLostR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR1Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.VODPacketsLostR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR2Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.VODPacketsLostR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR3Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.VODPacketsLostR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR4Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.VODPacketsLostR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR5Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.VODPacketsLostR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiBitRateR1Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.MultiBitRateR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiBitRateR2Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.MultiBitRateR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiBitRateR3Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.MultiBitRateR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiBitRateR4Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.MultiBitRateR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_MultiBitRateR5Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.MultiBitRateR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODBitRateR1Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.VODBitRateR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODBitRateR2Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.VODBitRateR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODBitRateR3Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.VODBitRateR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODBitRateR4Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.VODBitRateR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_VODBitRateR5Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.VODBitRateR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR1Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.FramesLostR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR2Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.FramesLostR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR3Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.FramesLostR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR4Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.FramesLostR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_FramesLostR5Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.FramesLostR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_PlayErrorInfo] = "Device.X_CTC_IPTV.ServiceStatistics.PlayErrorInfo";
		tr069_parameter_name_table[TR069_PARAM_MultiRRT] = "Device.X_CTC_IPTV.ServiceStatistics.MultiRRT";
		tr069_parameter_name_table[TR069_PARAM_VodRRT] = "Device.X_CTC_IPTV.ServiceStatistics.VodRRT";
		tr069_parameter_name_table[TR069_PARAM_HTTPRRT] = "Device.X_CTC_IPTV.ServiceStatistics.HTTPRRT";
		tr069_parameter_name_table[TR069_PARAM_MultiAbendNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.MultiAbendNumbers";
		tr069_parameter_name_table[TR069_PARAM_MultiAbendUPNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.MultiAbendUPNumbers";
		tr069_parameter_name_table[TR069_PARAM_VODUPAbendNumbers] = "Device.X_CTC_IPTV.ServiceStatistics.VODUPAbendNumbers";
		tr069_parameter_name_table[TR069_PARAM_FECMultiPacketsLostR1Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.FECMultiPacketsLostR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_FECMultiPacketsLostR2Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.FECMultiPacketsLostR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_FECMultiPacketsLostR3Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.FECMultiPacketsLostR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_FECMultiPacketsLostR4Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.FECMultiPacketsLostR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_FECMultiPacketsLostR5Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.FECMultiPacketsLostR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_ARQVODPacketsLostR1Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.ARQVODPacketsLostR1Nmb";
		tr069_parameter_name_table[TR069_PARAM_ARQVODPacketsLostR2Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.ARQVODPacketsLostR2Nmb";
		tr069_parameter_name_table[TR069_PARAM_ARQVODPacketsLostR3Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.ARQVODPacketsLostR3Nmb";
		tr069_parameter_name_table[TR069_PARAM_ARQVODPacketsLostR4Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.ARQVODPacketsLostR4Nmb";
		tr069_parameter_name_table[TR069_PARAM_ARQVODPacketsLostR5Nmb] = "Device.X_CTC_IPTV.ServiceStatistics.ARQVODPacketsLostR5Nmb";
		tr069_parameter_name_table[TR069_PARAM_BufferDecNmb] = "Device.X_CTC_IPTV.ServiceStatistics.BufferDecNmb";
		tr069_parameter_name_table[TR069_PARAM_BufferIncNmb] = "Device.X_CTC_IPTV.ServiceStatistics.BufferIncNmb";
	    tr069_parameter_name_table[TR069_PARAM_LogMsgEnable] = "Device.X_CTC_IPTV.LogMsg.Enable";
		tr069_parameter_name_table[TR069_PARAM_LogMsgOrFile] = "Device.X_CTC_IPTV.LogMsg.MsgOrFile";
		tr069_parameter_name_table[TR069_PARAM_LogMsgFtpServer] = "Device.X_CTC_IPTV.LogMsg.LogFtpServer";
		tr069_parameter_name_table[TR069_PARAM_LogMsgFtpUser] = "Device.X_CTC_IPTV.LogMsg.LogFtpUser";
		tr069_parameter_name_table[TR069_PARAM_LogMsgFtpPassword] = "Device.X_CTC_IPTV.LogMsg.LogFtpPassword";
		tr069_parameter_name_table[TR069_PARAM_LogMsgDuration] = "Device.X_CTC_IPTV.LogMsg.Duration";
		tr069_parameter_name_table[TR069_PARAM_LogMsgRTSPInfo] = "Device.X_CTC_IPTV.LogMsg.RTSPInfo";
		tr069_parameter_name_table[TR069_PARAM_LogMsgHTTPInfo] = "Device.X_CTC_IPTV.LogMsg.HTTPInfo";
		tr069_parameter_name_table[TR069_PARAM_LogMsgIGMPInfo] = "Device.X_CTC_IPTV.LogMsg.IGMPInfo";
		tr069_parameter_name_table[TR069_PARAM_LogMsgPkgTotalOneSec] = "Device.X_CTC_IPTV.LogMsg.PkgTotalOneSec";
		tr069_parameter_name_table[TR069_PARAM_LogMsgByteTotalOneSec] = "Device.X_CTC_IPTV.LogMsg.ByteTotalOneSec";
		tr069_parameter_name_table[TR069_PARAM_LogMsgPkgLostRate] = "Device.X_CTC_IPTV.LogMsg.PkgLostRate";
		tr069_parameter_name_table[TR069_PARAM_LogMsgAvarageRate] = "Device.X_CTC_IPTV.LogMsg.AvarageRate";
		tr069_parameter_name_table[TR069_PARAM_LogMsgBUFFER] = "Device.X_CTC_IPTV.LogMsg.BUFFER";
		tr069_parameter_name_table[TR069_PARAM_LogMsgERROR] = "Device.X_CTC_IPTV.LogMsg.ERROR";
		tr069_parameter_name_table[TR069_PARAM_LogMsgVendorExt] = "Device.X_CTC_IPTV.LogMsg.VendorExt";
		tr069_parameter_name_table[TR069_PARAM_OperatorInfo] = "";
		tr069_parameter_name_table[TR069_PARAM_IntegrityCheck] = "";
		tr069_parameter_name_table[TR069_PARAM_UpgradeURL] = "";
		tr069_parameter_name_table[TR069_PARAM_BrowserURL2] = "";
		tr069_parameter_name_table[TR069_PARAM_BrowserTagURL] = "";
		tr069_parameter_name_table[TR069_PARAM_AdministratorPassword] = "";
		tr069_parameter_name_table[TR069_PARAM_CUserPassword] = "";
		tr069_parameter_name_table[TR069_PARAM_UserProvince] = "";
		tr069_parameter_name_table[TR069_PARAM_PhyMemSize] = "Device.X_CTC_IPTV.PhyMemSize";
		tr069_parameter_name_table[TR069_PARAM_StorageSize] = "Device.X_CTC_IPTV.StorageSize";
		tr069_parameter_name_table[TR069_PARAM_AlarmReason] ="";
		tr069_parameter_name_table[TR069_PARAM_AlarmPacketsLostRate] ="";
		tr069_parameter_name_table[TR069_PARAM_AlarmFramesLost] ="";
		tr069_parameter_name_table[TR069_PARAM_TvmsgResServer] ="Device.X_FV.TVMS_IP";
		tr069_parameter_name_table[TR069_PARAM_TvmsgResInterval] ="Device.X_FV.RES_POLL_INTERVAL";
		tr069_parameter_name_table[TR069_PARAM_SQMEnableEPG] = "Device.X_FV.SQM.EnableEPG";
		tr069_parameter_name_table[TR069_PARAM_SQMEnableMedia] = "Device.X_FV.SQM.EnableMedia";
		tr069_parameter_name_table[TR069_PARAM_SQMEnableMediaBeforeEC] = "Device.X_FV.SQM.EnableMediaBeforeEC";
		tr069_parameter_name_table[TR069_PARAM_SQMEnableWarning] = "Device.X_FV.SQM.EnableWarning";
		tr069_parameter_name_table[TR069_PARAM_SQMEnableTelnet] = "Device.X_FV.SQM.EnableTelnet";
		tr069_parameter_name_table[TR069_PARAM_SQMMediaInterval] = "Device.X_FV.SQM.MediaInterval";
		tr069_parameter_name_table[TR069_PARAM_SQMServerAddress] = "Device.X_FV.SQM.ServerAddress";
		tr069_parameter_name_table[TR069_PARAM_SQMServerPort] = "Device.X_FV.SQM.ServerPort";
		tr069_parameter_name_table[TR069_PARAM_ConfigFile] = "Device.Config.ConfigFile";
		tr069_parameter_name_table[TR069_PARAM_CurrentLanguage] = "Device.UserInterface.CurrentLanguage";
		tr069_parameter_name_table[TR069_PARAM_AvailableLanguages] = "Device.UserInterface.AvailableLanguages";
		tr069_parameter_name_table[TR069_PARAM_IsSupportIPv6] = "Device.LAN.IsSupportIPv6";
		tr069_parameter_name_table[TR069_PARAM_SpeedTestEnable] = "Device.X_FV.Diagnostic.SpeedTestEnable";
		tr069_parameter_name_table[TR069_PARAM_SpeedTestDuration] = "Device.X_FV.Diagnostic.SpeedTestDuration";
		tr069_parameter_name_table[TR069_PARAM_SpeedTestPath] = "Device.X_FV.Diagnostic.SpeedTestPath";
		tr069_parameter_name_table[TR069_PARAM_SpeedTestUniqueNum] = "Device.X_FV.Diagnostic.SpeedTestUniqueNum";
		tr069_parameter_name_table[TR069_PARAM_SpeedTestResult] = "Device.X_FV.Diagnostic.SpeedTestResult";
		tr069_parameter_name_table[TR069_PARAM_SpeedTestCode] = "Device.X_FV.Diagnostic.SpeedTestCode";
		tr069_parameter_name_table[TR069_PARAM_PacketLostTestEnable] = "Device.X_FV.Diagnostic.PacketLostTestEnable";
		tr069_parameter_name_table[TR069_PARAM_PacketLostTestUDPPort] = "Device.X_FV.Diagnostic.PacketLostTestUDPPort";
		tr069_parameter_name_table[TR069_PARAM_PacketLostTestBand] = "Device.X_FV.Diagnostic.PacketLostTestBand";
		tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoPPPOEEnable] = "";
		tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoPPPOEID2 ] = "";
		tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoPPPOEPassword2] = "";
		tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoUserID2] = "";
		tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoUserIDPassword2] = "";
	}
}


static VOID_T tr069_parameter_idstack_clear(tr069_parameter_mgr *param_handle)
{
	param_handle->idstack_top = 0;
	param_handle->idstack[param_handle->idstack_top] = 0;

	return;
}

/*****************************************************************************************
Function:
	push and id to idstack

Input:
	param_handle: handle to parameter module involved
	id: id to push into idstack, next parameter will acquire this id

Output:

Return:
	-1: push fail
	0: push success
*****************************************************************************************/
static INT32_T tr069_parameter_idstack_push(tr069_parameter_mgr *param_handle, UINT16_T id)
{
	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (param_handle->idstack_top >= TR069_PARAM_IDSTACK_SIZE) {
		/* stack full, this does not happen generally
		   to avoid, set TR069_PARAM_IDSTACK_SIZE = TR069_MAX_PARAM_NUM */
		return -1;
	}

	param_handle->idstack[param_handle->idstack_top] = id;
	param_handle->idstack_top++;

	return 0;
}

static INT32_T tr069_parameter_idstack_pop(tr069_parameter_mgr *param_handle, UINT16_T *id)
{
	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (param_handle->idstack_top < 1) {
		/* this is an error state */
		return -1;
	}

	param_handle->idstack_top--;
	*id = param_handle->idstack[param_handle->idstack_top];

	if (param_handle->idstack_top == 0) {
		tr069_parameter_idstack_push(param_handle, *id + 1);
	}

	return 0;
}

static INT32_T tr069_parameter_init_params(tr069_parameter_mgr *parameter_handle)
{
	tr069_debug("enter\n");
	if (NULL == parameter_handle) {
		tr069_error("parameter handle error\n");
		return -1;
	}
	
	/* add all parameters from config */
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_Manufacturer], TR069_PARAM_Manufacturer, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ManufacturerOUI], TR069_PARAM_ManufacturerOUI, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ProductClass], TR069_PARAM_ProductClass, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SerialNumber], TR069_PARAM_SerialNumber, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HardwareVersion], TR069_PARAM_HardwareVersion, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SoftwareVersion], TR069_PARAM_SoftwareVersion, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ProvisioningCode], TR069_PARAM_ProvisioningCode, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_DeviceStatus], TR069_PARAM_DeviceStatus, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_KernelVersion], TR069_PARAM_KernelVersion, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FsVersion], TR069_PARAM_FsVersion, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AppVersion], TR069_PARAM_AppVersion, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_StreamingControlProtocols], TR069_PARAM_StreamingControlProtocols, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_StreamingTransportProtocols], TR069_PARAM_StreamingTransportProtocols, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_StreamingTransportControlProtocols], TR069_PARAM_StreamingTransportControlProtocols, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_DownloadTransportProtocols], TR069_PARAM_DownloadTransportProtocols, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiplexTypes], TR069_PARAM_MultiplexTypes, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MaxDejitteringBufferSize], TR069_PARAM_MaxDejitteringBufferSize, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AudioStandards], TR069_PARAM_AudioStandards, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VideoStandards], TR069_PARAM_VideoStandards, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
    
	// now for all standards
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_UDPConnectionRequestAddress], TR069_PARAM_UDPConnectionRequestAddress, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_UDPConnectionRequestNtfLimit], TR069_PARAM_UDPConnectionRequestNtfLimit, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_STUNEnable], TR069_PARAM_STUNEnable, TR069_DATATYPE_BOOLEAN, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_STUNServerAddress], TR069_PARAM_STUNServerAddress, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_STUNServerPort], TR069_PARAM_STUNServerPort, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_STUNUsername], TR069_PARAM_STUNUsername, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_STUNPassword], TR069_PARAM_STUNPassword, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_STUNMaxKeepAlivePeriod], TR069_PARAM_STUNMaxKeepAlivePeriod, TR069_DATATYPE_INT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_STUNMinKeepAlivePeriod], TR069_PARAM_STUNMinKeepAlivePeriod, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_NATDetected], TR069_PARAM_NATDetected, TR069_DATATYPE_BOOLEAN, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ManageSerURL], TR069_PARAM_ManageSerURL, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ManageSerUsername], TR069_PARAM_ManageSerUsername, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ManageSerPassword], TR069_PARAM_ManageSerPassword, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ConnectionRequestURL], TR069_PARAM_ConnectionRequestURL, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ConnectionRequestUsername], TR069_PARAM_ConnectionRequestUsername, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ConnectionRequestPassword], TR069_PARAM_ConnectionRequestPassword, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PeriodicInformEnable], TR069_PARAM_PeriodicInformEnable, TR069_DATATYPE_BOOLEAN, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PeriodicInformInterval], TR069_PARAM_PeriodicInformInterval, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PeriodicInformTime], TR069_PARAM_PeriodicInformTime, TR069_DATATYPE_DATETIME, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_UpgradesManaged], TR069_PARAM_UpgradesManaged, TR069_DATATYPE_BOOLEAN, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ParameterKey], TR069_PARAM_ParameterKey, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_NTPServer1], TR069_PARAM_NTPServer1, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LocalTimeZone], TR069_PARAM_LocalTimeZone, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_CurrentLocalTime], TR069_PARAM_CurrentLocalTime, TR069_DATATYPE_DATETIME, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_Device], TR069_PARAM_Device, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_DhcpUser], TR069_PARAM_DhcpUser, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_DhcpPassword], TR069_PARAM_DhcpPassword, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AddressingType], TR069_PARAM_AddressingType, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_IPAddressReal], TR069_PARAM_IPAddressReal, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SubnetMaskReal], TR069_PARAM_SubnetMaskReal, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_DefaultGatewayReal], TR069_PARAM_DefaultGatewayReal, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer0Real], TR069_PARAM_DNSServer0Real, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_DNSServer1Real], TR069_PARAM_DNSServer1Real, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MACAddress], TR069_PARAM_MACAddress, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ConnectionUpTime], TR069_PARAM_ConnectionUpTime, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TotalBytesSent], TR069_PARAM_TotalBytesSent, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TotalBytesReceived], TR069_PARAM_TotalBytesReceived, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TotalPacketsSent], TR069_PARAM_TotalPacketsSent, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TotalPacketsReceived], TR069_PARAM_TotalPacketsReceived, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_CurrentDayInterval], TR069_PARAM_CurrentDayInterval, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_CurrentDayBytesSent], TR069_PARAM_CurrentDayBytesSent, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_CurrentDayBytesReceived], TR069_PARAM_CurrentDayBytesReceived, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_CurrentDayPacketsSent], TR069_PARAM_CurrentDayPacketsSent, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_CurrentDayPacketsReceived], TR069_PARAM_CurrentDayPacketsReceived, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_QuarterHourInterval], TR069_PARAM_QuarterHourInterval, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_QuarterHourBytesSent], TR069_PARAM_QuarterHourBytesSent, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_QuarterHourBytesReceived], TR069_PARAM_QuarterHourBytesReceived, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_QuarterHourPacketsSent], TR069_PARAM_QuarterHourPacketsSent, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_QuarterHourPacketsReceived], TR069_PARAM_QuarterHourPacketsReceived, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PingDiagState], TR069_PARAM_PingDiagState, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PingDiagHost], TR069_PARAM_PingDiagHost, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PingDiagReptNumber], TR069_PARAM_PingDiagReptNumber, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PingDiagTimeout], TR069_PARAM_PingDiagTimeout, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PingDiagDataBlockSize], TR069_PARAM_PingDiagDataBlockSize, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PingDiagDSCP], TR069_PARAM_PingDiagDSCP, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PingDiagSuccessCount], TR069_PARAM_PingDiagSuccessCount, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PingDiagFailureCount], TR069_PARAM_PingDiagFailureCount, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PingDiagAvgRespTime], TR069_PARAM_PingDiagAvgRespTime, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PingDiagMinRespTime], TR069_PARAM_PingDiagMinRespTime, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PingDiagMaxRespTime], TR069_PARAM_PingDiagMaxRespTime, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TracertDiagState], TR069_PARAM_TracertDiagState, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TracertDiagHost], TR069_PARAM_TracertDiagHost, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TracertDiagTimeout], TR069_PARAM_TracertDiagTimeout, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TracertDiagDataBlockSize], TR069_PARAM_TracertDiagDataBlockSize, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TracertDiagMaxHopCount], TR069_PARAM_TracertDiagMaxHopCount, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TracertDiagDSCP], TR069_PARAM_TracertDiagDSCP, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TracertDiagRespTime], TR069_PARAM_TracertDiagRespTime, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TracertDiagHopsNumber], TR069_PARAM_TracertDiagHopsNumber, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST1], TR069_PARAM_HOPHOST1, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST2], TR069_PARAM_HOPHOST2, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST3], TR069_PARAM_HOPHOST3, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST4], TR069_PARAM_HOPHOST4, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST5], TR069_PARAM_HOPHOST5, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST6], TR069_PARAM_HOPHOST6, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST7], TR069_PARAM_HOPHOST7, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST8], TR069_PARAM_HOPHOST8, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST9], TR069_PARAM_HOPHOST9, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST10], TR069_PARAM_HOPHOST10, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST11], TR069_PARAM_HOPHOST11, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST12], TR069_PARAM_HOPHOST12, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST13], TR069_PARAM_HOPHOST13, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST14], TR069_PARAM_HOPHOST14, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST15], TR069_PARAM_HOPHOST15, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST16], TR069_PARAM_HOPHOST16, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST17], TR069_PARAM_HOPHOST17, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST18], TR069_PARAM_HOPHOST18, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST19], TR069_PARAM_HOPHOST19, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST20], TR069_PARAM_HOPHOST20, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST21], TR069_PARAM_HOPHOST21, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST22], TR069_PARAM_HOPHOST22, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST23], TR069_PARAM_HOPHOST23, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST24], TR069_PARAM_HOPHOST24, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST25], TR069_PARAM_HOPHOST25, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST26], TR069_PARAM_HOPHOST26, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST27], TR069_PARAM_HOPHOST27, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST28], TR069_PARAM_HOPHOST28, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST29], TR069_PARAM_HOPHOST29, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST30], TR069_PARAM_HOPHOST30, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST31], TR069_PARAM_HOPHOST31, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HOPHOST32], TR069_PARAM_HOPHOST32, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_STBID], TR069_PARAM_STBID, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PPPoEID], TR069_PARAM_PPPoEID, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PPPoEPassword], TR069_PARAM_PPPoEPassword, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_UserID], TR069_PARAM_UserID, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_UserPassword], TR069_PARAM_UserPassword, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthURL], TR069_PARAM_AuthURL, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogServerUrl], TR069_PARAM_LogServerUrl, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogUploadInterval], TR069_PARAM_LogUploadInterval, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogRecordInterval], TR069_PARAM_LogRecordInterval, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_StatInterval], TR069_PARAM_StatInterval, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PacketsLostR1], TR069_PARAM_PacketsLostR1, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PacketsLostR2], TR069_PARAM_PacketsLostR2, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PacketsLostR3], TR069_PARAM_PacketsLostR3, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PacketsLostR4], TR069_PARAM_PacketsLostR4, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PacketsLostR5], TR069_PARAM_PacketsLostR5, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_BitRateR1], TR069_PARAM_BitRateR1, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_BitRateR2], TR069_PARAM_BitRateR2, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_BitRateR3], TR069_PARAM_BitRateR3, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_BitRateR4], TR069_PARAM_BitRateR4, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_BitRateR5], TR069_PARAM_BitRateR5, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FramesLostR1], TR069_PARAM_FramesLostR1, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FramesLostR2], TR069_PARAM_FramesLostR2, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FramesLostR3], TR069_PARAM_FramesLostR3, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FramesLostR4], TR069_PARAM_FramesLostR4, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FramesLostR5], TR069_PARAM_FramesLostR5, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_Startpoint], TR069_PARAM_Startpoint, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_Endpoint], TR069_PARAM_Endpoint, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthNumbers], TR069_PARAM_AuthNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthFailNumbers], TR069_PARAM_AuthFailNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthFailInfo], TR069_PARAM_AuthFailInfo, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiReqNumbers], TR069_PARAM_MultiReqNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiFailNumbers], TR069_PARAM_MultiFailNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiFailInfo], TR069_PARAM_MultiFailInfo, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VodReqNumbers], TR069_PARAM_VodReqNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VodFailNumbers], TR069_PARAM_VodFailNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VodFailInfo], TR069_PARAM_VodFailInfo, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HTTPReqNumbers], TR069_PARAM_HTTPReqNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HTTPFailNumbers], TR069_PARAM_HTTPFailNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HTTPFailInfo], TR069_PARAM_HTTPFailInfo, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MutiAbendNumbers], TR069_PARAM_MutiAbendNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODAbendNumbers], TR069_PARAM_VODAbendNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PlayErrorNumbers], TR069_PARAM_PlayErrorNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR1Nmb], TR069_PARAM_MultiPacketsLostR1Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR2Nmb], TR069_PARAM_MultiPacketsLostR2Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR3Nmb], TR069_PARAM_MultiPacketsLostR3Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR4Nmb], TR069_PARAM_MultiPacketsLostR4Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiPacketsLostR5Nmb], TR069_PARAM_MultiPacketsLostR5Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR1Nmb], TR069_PARAM_VODPacketsLostR1Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR2Nmb], TR069_PARAM_VODPacketsLostR2Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR3Nmb], TR069_PARAM_VODPacketsLostR3Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR4Nmb], TR069_PARAM_VODPacketsLostR4Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODPacketsLostR5Nmb], TR069_PARAM_VODPacketsLostR5Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiBitRateR1Nmb], TR069_PARAM_MultiBitRateR1Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiBitRateR2Nmb], TR069_PARAM_MultiBitRateR2Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiBitRateR3Nmb], TR069_PARAM_MultiBitRateR3Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiBitRateR4Nmb], TR069_PARAM_MultiBitRateR4Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiBitRateR5Nmb], TR069_PARAM_MultiBitRateR5Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODBitRateR1Nmb], TR069_PARAM_VODBitRateR1Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODBitRateR2Nmb], TR069_PARAM_VODBitRateR2Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODBitRateR3Nmb], TR069_PARAM_VODBitRateR3Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODBitRateR4Nmb], TR069_PARAM_VODBitRateR4Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODBitRateR5Nmb], TR069_PARAM_VODBitRateR5Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FramesLostR1Nmb], TR069_PARAM_FramesLostR1Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FramesLostR2Nmb], TR069_PARAM_FramesLostR2Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FramesLostR3Nmb], TR069_PARAM_FramesLostR3Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FramesLostR4Nmb], TR069_PARAM_FramesLostR4Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FramesLostR5Nmb], TR069_PARAM_FramesLostR5Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PlayErrorInfo], TR069_PARAM_PlayErrorInfo, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);

	if (TR069_STAND_CTC == g_tr069_stand) {
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgEnable], TR069_PARAM_LogMsgEnable, TR069_DATATYPE_BOOLEAN, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgOrFile], TR069_PARAM_LogMsgOrFile, TR069_DATATYPE_BOOLEAN, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgFtpServer], TR069_PARAM_LogMsgFtpServer, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgFtpUser], TR069_PARAM_LogMsgFtpUser, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgFtpPassword], TR069_PARAM_LogMsgFtpPassword, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgDuration], TR069_PARAM_LogMsgDuration, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgRTSPInfo], TR069_PARAM_LogMsgRTSPInfo, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgHTTPInfo], TR069_PARAM_LogMsgHTTPInfo, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgIGMPInfo], TR069_PARAM_LogMsgIGMPInfo, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgPkgTotalOneSec], TR069_PARAM_LogMsgPkgTotalOneSec, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgByteTotalOneSec], TR069_PARAM_LogMsgByteTotalOneSec, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgPkgLostRate], TR069_PARAM_LogMsgPkgLostRate, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgAvarageRate], TR069_PARAM_LogMsgAvarageRate, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgBUFFER], TR069_PARAM_LogMsgBUFFER, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgERROR], TR069_PARAM_LogMsgERROR, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_LogMsgVendorExt], TR069_PARAM_LogMsgVendorExt, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ModelName], TR069_PARAM_ModelName, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ModelID], TR069_PARAM_ModelID, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_Description ], TR069_PARAM_Description, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ConfigFileVersion], TR069_PARAM_ConfigFileVersion, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_UpTime], TR069_PARAM_UpTime, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FirstUseDate], TR069_PARAM_FirstUseDate, TR069_DATATYPE_DATETIME, TR069_NOTIFICATION_OFF, 0);
	
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_DeviceLog], TR069_PARAM_DeviceLog, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ManageSerURLBackup], TR069_PARAM_ManageSerURLBackup, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ConfigFile], TR069_PARAM_ConfigFile, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AvailableLanguages], TR069_PARAM_AvailableLanguages, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_CurrentLanguage], TR069_PARAM_CurrentLanguage, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_IsSupportIPv6], TR069_PARAM_IsSupportIPv6, TR069_DATATYPE_BOOLEAN, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiRRT], TR069_PARAM_MultiRRT, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VodRRT], TR069_PARAM_VodRRT, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_HTTPRRT], TR069_PARAM_HTTPRRT, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
    tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiAbendUPNumbers], TR069_PARAM_MultiAbendNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_MultiAbendUPNumbers], TR069_PARAM_MultiAbendUPNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_VODUPAbendNumbers], TR069_PARAM_VODUPAbendNumbers, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FECMultiPacketsLostR1Nmb], TR069_PARAM_FECMultiPacketsLostR1Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FECMultiPacketsLostR2Nmb], TR069_PARAM_FECMultiPacketsLostR2Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FECMultiPacketsLostR3Nmb], TR069_PARAM_FECMultiPacketsLostR3Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FECMultiPacketsLostR4Nmb], TR069_PARAM_FECMultiPacketsLostR4Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_FECMultiPacketsLostR5Nmb], TR069_PARAM_FECMultiPacketsLostR5Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ARQVODPacketsLostR1Nmb], TR069_PARAM_ARQVODPacketsLostR1Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ARQVODPacketsLostR2Nmb], TR069_PARAM_ARQVODPacketsLostR2Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ARQVODPacketsLostR3Nmb], TR069_PARAM_ARQVODPacketsLostR3Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ARQVODPacketsLostR4Nmb], TR069_PARAM_ARQVODPacketsLostR4Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ARQVODPacketsLostR5Nmb], TR069_PARAM_ARQVODPacketsLostR5Nmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_BufferDecNmb], TR069_PARAM_BufferDecNmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_BufferIncNmb], TR069_PARAM_BufferIncNmb, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SpeedTestEnable], TR069_PARAM_SpeedTestEnable, TR069_DATATYPE_BOOLEAN, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SpeedTestDuration], TR069_PARAM_SpeedTestDuration, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SpeedTestPath], TR069_PARAM_SpeedTestPath, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SpeedTestUniqueNum], TR069_PARAM_SpeedTestUniqueNum, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SpeedTestCode], TR069_PARAM_SpeedTestCode, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SpeedTestResult], TR069_PARAM_SpeedTestResult, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PacketLostTestEnable], TR069_PARAM_PacketLostTestEnable, TR069_DATATYPE_BOOLEAN, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PacketLostTestUDPPort], TR069_PARAM_PacketLostTestUDPPort, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PacketLostTestBand], TR069_PARAM_PacketLostTestBand, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	}

	if (TR069_STAND_CU == g_tr069_stand) {
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoPPPOEEnable], TR069_PARAM_AuthServiceInfoPPPOEEnable, TR069_DATATYPE_BOOLEAN, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoPPPOEID2], TR069_PARAM_AuthServiceInfoPPPOEID2, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoPPPOEPassword2], TR069_PARAM_AuthServiceInfoPPPOEPassword2, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoUserID2], TR069_PARAM_AuthServiceInfoUserID2, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthServiceInfoUserIDPassword2], TR069_PARAM_AuthServiceInfoUserIDPassword2, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_OperatorInfo], TR069_PARAM_OperatorInfo, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_IntegrityCheck], TR069_PARAM_IntegrityCheck, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_UpgradeURL], TR069_PARAM_UpgradeURL, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_BrowserURL2], TR069_PARAM_BrowserURL2, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_BrowserTagURL], TR069_PARAM_BrowserTagURL, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AdministratorPassword], TR069_PARAM_AdministratorPassword, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_CUserPassword], TR069_PARAM_CUserPassword, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_UserProvince], TR069_PARAM_UserProvince, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AlarmReason], TR069_PARAM_AlarmReason, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AlarmPacketsLostRate], TR069_PARAM_AlarmPacketsLostRate, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AlarmFramesLost], TR069_PARAM_AlarmFramesLost, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	}

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TvmsgResServer], TR069_PARAM_TvmsgResServer, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_TvmsgResInterval], TR069_PARAM_TvmsgResInterval, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);

	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthURLBackup], TR069_PARAM_AuthURLBackup, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SQMEnableEPG], TR069_PARAM_SQMEnableEPG, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SQMEnableMedia], TR069_PARAM_SQMEnableMedia, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SQMEnableMediaBeforeEC], TR069_PARAM_SQMEnableMediaBeforeEC, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SQMEnableWarning], TR069_PARAM_SQMEnableWarning, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SQMEnableTelnet], TR069_PARAM_SQMEnableTelnet, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SQMMediaInterval], TR069_PARAM_SQMMediaInterval, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SQMServerAddress], TR069_PARAM_SQMServerAddress, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_SQMServerPort], TR069_PARAM_SQMServerPort, TR069_DATATYPE_UNSIGNEDINT, TR069_NOTIFICATION_OFF, 1);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_PhyMemSize], TR069_PARAM_PhyMemSize, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	tr069_parameter_add_param(parameter_handle, tr069_parameter_name_table[TR069_PARAM_StorageSize], TR069_PARAM_StorageSize, TR069_DATATYPE_STRING, TR069_NOTIFICATION_OFF, 0);
	
	/* set default parameters value change upload */
	if (g_tr069_stand == TR069_STAND_CU) {
		char config_cmds[F_CFG_NUM + 1];
		int i;
		for(i = 0; i < F_CFG_NUM; i++) {
			config_cmds[i] = '0';
		}
		config_cmds[F_CFG_ManageSerURL] = '1';
		config_cmds[F_CFG_STBID] = '1';
		config_cmds[F_CFG_OperatorInfo] = '1';
		config_cmds[F_CFG_UpgradeDomain] = '1';
		config_cmds[F_CFG_AuthURL] = '1';
		config_cmds[F_CFG_AuthURLBackup] = '1';
		config_cmds[F_CFG_UserProvince] = '1';
		config_cmds[F_CFG_NUM] = '\0';

		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_OperatorInfo], "2", 1, 0);
		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_UpgradeURL], "2", 1, 0);
		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_UserProvince], "2", 1, 0);
		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_BrowserURL2], "2", 1, 0);

		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthURLBackup], "2", 1, 0);
		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthURL], "2", 1, 0);
		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ManageSerURL], "2", 1, 0);
		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_STBID], "2", 1, 0);
		
		n_cfg_set_parameter_attribute(config_cmds);
	} else if (g_tr069_stand == TR069_STAND_CTC) {
		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthURLBackup], "2", 1, 0);
		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_AuthURL], "2", 1, 0);
		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_ManageSerURL], "2", 1, 0);
		tr069_parameter_set_attribute(parameter_handle, tr069_parameter_name_table[TR069_PARAM_STBID], "2", 1, 0);
	}


	return 0;
}


tr069_parameter_mgr* tr069_parameter_init(tr069_mgr *tr069_handle)
{
	tr069_parameter_mgr *manager = NULL;

	tr069_debug("enter\n");
	if (NULL == tr069_handle) {
		tr069_error("tr069 handle error\n");
		return NULL;
	}

	manager = malloc(sizeof(tr069_parameter_mgr));
	if (NULL == manager) {
		tr069_error("malloc parameter manager error\n");
		return NULL;
	}
	memset(manager, 0, sizeof(tr069_parameter_mgr));

	/* initialize all elements */
	manager->tr069_handle = tr069_handle;
	manager->param_number = 1;

	/* set root */
	manager->param_root.child = NULL;
	manager->param_root.child_number = 0;
	manager->param_root.data_type = TR069_DATATYPE_OBJECT;
	manager->param_root.write_flag = 0;
	manager->param_root.next = NULL;
	/* id "0" is reserved to root */
	manager->param_root.param_id = 0;
	manager->param_root.param_name = "Device.";
//	manager->param_root.param_value = NULL;

	memset(manager->param_table, 0, sizeof(manager->param_table));
	manager->param_table[0] = &(manager->param_root);

	tr069_parameter_init_name_table();

	tr069_parameter_idstack_clear(manager);

	/* first id to be allocated is "1" */
	tr069_parameter_idstack_push(manager, 1);

	tr069_parameter_init_params(manager);

	tr069_debug("ok quit\n");
	return manager;

	goto ERROR_EXIT;

ERROR_EXIT:

	/* if parameter manager already initialized, uninitialize it */
	if (manager != NULL) {
		manager = tr069_parameter_uninit(manager);
	}

	tr069_error("error quit\n");
	return NULL;
}

tr069_parameter_mgr* tr069_parameter_uninit(tr069_parameter_mgr *param_handle)
{
	UINT16_T i;

	tr069_debug("enter\n");
	if (param_handle != NULL) {
		/* free all allocated parameters excluding root */
		for (i = 1; i < (param_handle->param_number + param_handle->idstack_top - 1); i++) {
			if (param_handle->param_table[i] != NULL) {
				free(param_handle->param_table[i]);
				param_handle->param_table[i] = NULL;
			}
		}

		param_handle->param_table[0] = NULL;
		memset(&(param_handle->param_root), 0, sizeof(tr069_parameter));
		param_handle->param_number = 0;

		/* maybe unnecessary to do this */
		memset(param_handle->param_table, 0, sizeof(param_handle->param_table));

		tr069_parameter_idstack_clear(param_handle);

		param_handle->tr069_handle = NULL;

		free(param_handle);
	}

	tr069_debug("ok quit\n");
	return NULL;
}


tr069_parameter* tr069_parameter_add_param(tr069_parameter_mgr *param_handle, INT8_T *param_name, INT32_T param_index, tr069_data_type param_type, tr069_notification_type notif_type, UINT8_T write_flag)
{
	tr069_parameter *temp_param;
	tr069_parameter **append_param;
	UINT16_T i;
	UINT16_T name_len;
//	UINT16_T config_name_len;
	UINT8_T node_found_flag;

	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return NULL;
	}

	if (NULL == param_name) {
		tr069_error("parameter name error\n");
		return NULL;
	}

	if (TR069_DATATYPE_OBJECT == param_type) {
		tr069_error("parameter type error\n");
		return NULL;
	}

	if (param_handle->param_number >= TR069_MAX_PARAM_NUM) {
		/* parameter full */
		tr069_error("parameter reach max limit\n");
		return NULL;
	}

	name_len = strlen(param_name);
//	config_name_len = strlen(param_index);
	if (name_len > TR069_MAX_PARAM_NAME_LEN) {
		/* parameter name too long */
		tr069_error("name(%s) too long\n", param_name);
		return NULL;
	}

	if ('.' == param_name[name_len - 1]) {
		/* parameter name can not end with "." */
		tr069_error("parameter name(%s) invalid\n", param_name);
		return NULL;
	}

	append_param = NULL;
	temp_param = &(param_handle->param_root);
	for (i = 0; i < name_len; i++) {
		if ('.' == param_name[i]) {
			/* find for matching middle node in current layer */
			node_found_flag = 0;
			while (temp_param != NULL) {
				if (memcmp(temp_param->param_name, param_name, i) == 0) {
					/* there is matching middle node */
					node_found_flag = 1;
					break;
				}

				append_param = &(temp_param->next);
				temp_param = temp_param->next;
			}

			if (0 == node_found_flag) {
				if (append_param != NULL) {
					/* there isn't matching middle node, create new */
					/* best solution is to cache all node and add them once after verification */
					if (param_handle->param_number >= TR069_MAX_PARAM_NUM) {
						/* parameter full */
						tr069_error("parameter reach max limit\n");
						return NULL;
					}

					*append_param = malloc(sizeof(tr069_parameter) + i + 2);
					(*append_param)->data_type = TR069_DATATYPE_OBJECT;
					(*append_param)->child = NULL;
					(*append_param)->child_number = 0;
					(*append_param)->notif_type = TR069_NOTIFICATION_OFF;
					(*append_param)->write_flag = 0;
					(*append_param)->next = NULL;

					tr069_parameter_idstack_pop(param_handle, &((*append_param)->param_id));
					param_handle->param_table[(*append_param)->param_id] = *append_param;
					param_handle->param_number++;

					(*append_param)->param_name = (INT8_T*)(*append_param) + sizeof(tr069_parameter);
					memcpy((*append_param)->param_name, param_name, i + 1);
					(*append_param)->param_name[i + 1] = 0;

//					(*append_param)->value_len = 0;
					//(*append_param)->param_index = NULL;

					/* should modify parent node's child_num */

					temp_param = *append_param;
				} else {
					/* only when &param_root is NULL comes here */
					return NULL;
				}
			}

			/* layer down */
			append_param = &(temp_param->child);
			temp_param = temp_param->child;
		}
	}

	/* implement actual parameter node adding operation */
	/* best solution is to cache all node and add them once after verification */
	if (param_handle->param_number >= TR069_MAX_PARAM_NUM) {
		/* parameter full */
		tr069_error("parameter reach max limit\n");
		return NULL;
	}

	/* move to append position */
	while (*append_param != NULL) {
		if (strcmp(param_name, (*append_param)->param_name) == 0) {
			tr069_error("parameter(%s) already exsist\n", param_name);
			return NULL;
		}

		append_param = &((*append_param)->next);
	}

	*append_param = malloc(sizeof(tr069_parameter) + name_len + 1);

	(*append_param)->data_type = param_type;
	(*append_param)->child = NULL;
	(*append_param)->child_number = 0;
	(*append_param)->notif_type = notif_type;
	(*append_param)->write_flag = write_flag;
	(*append_param)->next = NULL;

	tr069_parameter_idstack_pop(param_handle, &((*append_param)->param_id));
	param_handle->param_table[(*append_param)->param_id] = *append_param;
	param_handle->param_number++;

	(*append_param)->param_name = (INT8_T*)(*append_param) + sizeof(tr069_parameter);
	memcpy((*append_param)->param_name, param_name, name_len);
	(*append_param)->param_name[name_len] = 0;

//	(*append_param)->value_len = 0;
	(*append_param)->param_index = param_index;

	/* should also modify parent's child_num */
	return *append_param;
}

INT32_T tr069_parameter_get_param(tr069_parameter_mgr *param_handle, INT8_T *param_name,
                                  tr069_parameter **param_set, UINT8_T next_level, UINT8_T object_flag)
{
	tr069_parameter *source_param;
	INT16_T param_num;

	tr069_debug("param_name=%s next_level=%d\n", param_name, next_level);
	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (NULL == param_name) {
		tr069_error("parameter name error\n");
		return -1;
	}

	if (NULL == param_set) {
		tr069_error("parameter receive buffer error\n");
		return -1;
	}

	if (strlen(param_name) != 0) {
		source_param = tr069_parameter_find_node(param_handle, param_name, NULL);
	} else {
		if (1 == next_level) {
			param_set[0] = &(param_handle->param_root);
			return 1;
		} else {
			source_param = &(param_handle->param_root);
		}
	}

	if (NULL == source_param) {
		tr069_error("find parameter error\n");
		return -1;
	}

	if (0 == next_level) {
		param_num = 0;
		tr069_parameter_recursive_get(param_handle, source_param, param_set, (UINT16_T *)&param_num, object_flag);
	} else {
		param_num = 0;
		source_param = source_param->child;

		while (source_param != NULL) {
			if (1 == object_flag || source_param->data_type != TR069_DATATYPE_OBJECT) {
				param_set[param_num] = source_param;
				param_num++;
			}
			source_param = source_param->next;
		}
	}

	tr069_debug("found\n");
	return param_num;
}

tr069_parameter* tr069_parameter_find_node(tr069_parameter_mgr *param_handle, INT8_T *param_name, tr069_parameter ***ref_pointer)
{
	tr069_parameter **temp_pointer;
	tr069_parameter *temp_param;
	UINT16_T i;
	UINT16_T name_len;
	UINT8_T node_found_flag;

	tr069_debug("param_name=%s\n", param_name);
	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return NULL;
	}

	if (NULL == param_name) {
		tr069_error("parameter name error\n");
		return NULL;
	}

	name_len = strlen(param_name);
	if (name_len > TR069_MAX_PARAM_NAME_LEN) {
		tr069_error("parameter name too long\n");
		return NULL;
	}

	temp_pointer = NULL;
	temp_param = &(param_handle->param_root);
	for (i = 0; i < name_len; i++) {
		if ('.' == param_name[i] || (name_len - 1) == i) {
			node_found_flag = 0;
			while (temp_param != NULL) {
				if (memcmp(temp_param->param_name, param_name, i + 1) == 0) {
					node_found_flag = 1;
					break;
				}

				temp_pointer = &(temp_param->next);
				temp_param = temp_param->next;
			}

			if (0 == node_found_flag) {
				tr069_error("parameter prefix not found\n");
				return NULL;
			} else {
				if ((name_len - 1) == i && 0 == temp_param->param_name[name_len]) {
					tr069_debug("found\n");
					if (ref_pointer != NULL) {
						*ref_pointer = temp_pointer;
					}

					return temp_param;
				} else {
					temp_pointer = &(temp_param->child);
					temp_param = temp_param->child;
				}
			}
		}
	}

	tr069_error("unexpected routine\n");
	return NULL;
}

static VOID_T tr069_parameter_recursive_get(tr069_parameter_mgr *param_handle, tr069_parameter *param, tr069_parameter **param_set, UINT16_T *get_num, UINT8_T object_flag)
{
	tr069_debug("enter\n");

	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return;
	}

	if (NULL == param_set) {
		tr069_error("parameter receive buffer error\n");
		return;
	}

	if (NULL == get_num) {
		tr069_error("get number pointer error\n");
		return;
	}

	if (NULL == param) {
		tr069_error("parameter error\n");
		return;
	}

	if (1 == object_flag || param->data_type != TR069_DATATYPE_OBJECT) {
		param_set[*get_num] = param;
		(*get_num)++;
	}

	param = param->child;

	while (param != NULL) {
		tr069_debug("param_name=%s\n", param->param_name);
		tr069_parameter_recursive_get(param_handle, param, param_set, get_num, object_flag);
		param = param->next;
	}


	/*
	while (param != NULL)
	{
		if (object_flag == 1 || param->data_type != TR069_DATATYPE_OBJECT)
		{
			param_set[*get_num] = param;
			(*get_num)++;
		}

		tr069_parameter_recursive_get(param->child, param_set, get_num, object_flag);
		param = param->next;
	}
	*/

	return;
}

static VOID_T tr069_parameter_recursive_remove(tr069_parameter_mgr *param_handle, tr069_parameter *param)
{
	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return;
	}

	if (NULL == param) {
		tr069_error("parameter error\n");
		return;
	}

	if (param->child != NULL) {
		tr069_parameter_recursive_remove(param_handle, param->child);
	}

	if (param->next != NULL) {
		tr069_parameter_recursive_remove(param_handle, param->next);
	}

	if (param_handle->param_number <= 1) {
		tr069_error("parameter reach min limit\n");
		return;
	}

	if (tr069_parameter_idstack_push(param_handle, param->param_id) != -1) {
		param_handle->param_table[param->param_id] = NULL;
		param_handle->param_number--;
		free(param);
	}

	return;
}


INT32_T tr069_parameter_remove_param(tr069_parameter_mgr *param_handle, INT8_T *param_name)
{
	tr069_parameter **ref_pointer = NULL;
	tr069_parameter *source_param;

	tr069_debug("enter (%s)\n", param_name);
	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (NULL == param_name) {
		tr069_error("parameter name error\n");
		return -1;
	}

	source_param = tr069_parameter_find_node(param_handle, param_name, &ref_pointer);

	if (NULL == source_param) {
		tr069_error("find parameter error\n");
		return -1;
	}

	if (NULL == ref_pointer) {
		tr069_error("reference pointer error\n");
		return -1;
	}

	*(ref_pointer) = source_param->next;
	source_param->next = NULL;

	tr069_parameter_recursive_remove(param_handle, source_param);

	tr069_debug("ok quit\n");
	return 0;
}

INT32_T tr069_parameter_set_value(tr069_parameter_mgr *param_handle, INT8_T *param_name, INT8_T *param_value, UINT8_T force_flag)
{
	tr069_parameter *temp_param;
	INT32_T ret;
	UINT16_T i;
	UINT16_T name_len;
	UINT16_T value_len;
	UINT8_T node_found_flag;

	tr069_debug("enter name=%s value=%s\n", param_name, param_value);
	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (NULL == param_name) {
		tr069_error("parameter name error\n");
		return -1;
	}

	if (NULL == param_value) {
		tr069_error("parameter value error\n");
		return -2;
	}

	name_len = strlen(param_name);
	if (name_len > TR069_MAX_PARAM_NAME_LEN) {
		tr069_error("parameter name too long\n");
		return -1;
	}

	value_len = strlen(param_value);
	if (value_len > TR069_MAX_PARAM_VALUE_LEN) {
		tr069_error("parameter value too long\n");
		return -2;
	}

	if ('.' == param_name[name_len - 1]) {
		tr069_error("parameter name invalid\n");
		return -1;
	}

	temp_param = &(param_handle->param_root);
	for (i = 0; i < name_len; i++) {
		if ('.' == param_name[i]) {
			node_found_flag = 0;
			while (temp_param != NULL) {
				if (strlen(temp_param->param_name) == (i + 1) && memcmp(temp_param->param_name, param_name, i + 1) == 0) {
					node_found_flag = 1;
					break;
				}

				temp_param = temp_param->next;
			}

			if (0 == node_found_flag) {
				tr069_error("parameter prefix not found\n");
				return -1;
			} else {
				temp_param = temp_param->child;
			}
		} else if ((name_len - 1) == i) {
			node_found_flag = 0;
			while (temp_param != NULL) {
				if (strlen(temp_param->param_name) == name_len &&
				                memcmp(temp_param->param_name, param_name, name_len) == 0) {
					node_found_flag = 1;
					break;
				}

				temp_param = temp_param->next;
			}

			if (0 == node_found_flag) {
				tr069_error("parameter not found\n");
				return -1;
			} else {
				/* set parameter value */
				if (0 == temp_param->write_flag && (0 == force_flag || 2 == force_flag)) {
					tr069_error("parameter not writable\n");
					return -3;
				}

				/* check for type and value */
				if (TR069_DATATYPE_BOOLEAN == temp_param->data_type) {
					if (strncasecmp(param_value, "true", sizeof("true")) &&
						strncasecmp(param_value, "false", sizeof("false"))) {
						tr069_error("need a bool value\n");
						return -4;
					}
				}
				else if (TR069_DATATYPE_UNSIGNEDINT == temp_param->data_type) {
					if (!is_int(param_value)) {
						tr069_error("need an int value\n");
						return -4;
					}
					if (atoi(param_value) < 0) {
						tr069_error("int below zero\n");
						return -2;
					}
				}
				else if (TR069_DATATYPE_INT == temp_param->data_type) {
					if (!is_int(param_value)) {
						tr069_error("need an int value\n");
						return -4;
					}
				}
				else if (TR069_DATATYPE_DATETIME == temp_param->data_type) {
					if (-1 == tr069_unformat_time(param_value)) {
						tr069_error("need a datetime value\n");
						return -4;
					}
				}

				if (2 == force_flag) {
					tr069_debug("ok quit without set\n");
					return 0;
				}

				/* set parameter value to config.ini */
				ret = tr069_set_config(temp_param->param_index, param_value);
				if (0 == ret) {
					tr069_debug("ok quit\n");
					return 0;
				} else {
					tr069_error("tr069_set_config error\n");
					return -5;
				}
			}
		}
	}

	tr069_error("unexpected routine\n");
	return -5;
}

INT32_T tr069_parameter_set_attribute(tr069_parameter_mgr *param_handle, INT8_T *param_name, INT8_T *param_value, UINT8_T force_flag, UINT8_T mode_flag)
{
	tr069_parameter *temp_param;
	INT32_T ret;
	UINT16_T i;
	UINT16_T name_len;
	UINT16_T value_len;
	UINT8_T node_found_flag;

	tr069_debug("enter name=%s value=%s\n", param_name, param_value);
	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (NULL == param_name) {
		tr069_error("parameter name error\n");
		return -1;
	}

	if (NULL == param_value) {
		tr069_error("parameter value error\n");
		return -2;
	}

	name_len = strlen(param_name);
	if (name_len > TR069_MAX_PARAM_NAME_LEN) {
		tr069_error("parameter name too long\n");
		return -1;
	}

	value_len = strlen(param_value);
	if (value_len > TR069_MAX_PARAM_VALUE_LEN) {
		tr069_error("parameter value too long\n");
		return -2;
	}

	if ('.' == param_name[name_len - 1]) {
		tr069_error("parameter name invalid\n");
		return -1;
	}

	temp_param = &(param_handle->param_root);
	for (i = 0; i < name_len; i++) {
		if ('.' == param_name[i]) {
			node_found_flag = 0;
			while (temp_param != NULL) {
				if (strlen(temp_param->param_name) == (i + 1) && memcmp(temp_param->param_name, param_name, i + 1) == 0) {
					node_found_flag = 1;
					break;
				}

				temp_param = temp_param->next;
			}

			if (0 == node_found_flag) {
				tr069_error("parameter prefix not found\n");
				return -1;
			} else {
				temp_param = temp_param->child;
			}
		} else if ((name_len - 1) == i) {
			node_found_flag = 0;
			while (temp_param != NULL) {
				if (strlen(temp_param->param_name) == name_len &&
				                memcmp(temp_param->param_name, param_name, name_len) == 0) {
					node_found_flag = 1;
					break;
				}

				temp_param = temp_param->next;
			}

			if (0 == node_found_flag) {
				tr069_error("parameter not found\n");
				return -1;
			} else {
				/* set parameter value */
				if (0 == temp_param->write_flag && (0 == force_flag || 2 == force_flag)) {
					tr069_error("parameter not writable\n");
					return -3;
				}

				if (*param_value != '0' && *param_value != '1' && *param_value != '2') {
					tr069_error("attribute value error\n");
					return -2;
				}

				if (2 == force_flag) {
					tr069_debug("ok quit without set\n");
					return 0;
				}

				temp_param->notif_type = atoi(param_value);
				
				/* set parameter value to config.ini */
				if (mode_flag == 1) {
					ret = tr069_set_parameter_attribute(temp_param->param_index, param_value);
					
					if (0 != ret) {
						tr069_error("tr069_set_config error\n");
						return -5;	
					} else {
						tr069_debug("ok quit\n");
					}
				}
				
				return 0;
			}
		}
	}

	tr069_error("unexpected routine\n");
	return -5;
}


INT32_T tr069_parameter_get_value(tr069_parameter_mgr *param_handle, tr069_parameter *param, INT8_T *value_recver, INT32_T value_size)
{
	INT32_T ret;

	tr069_debug("enter\n");
	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (NULL == param) {
		tr069_error("parameter error\n");
		return -1;
	}

	if (NULL == value_recver) {
		tr069_error("value receive buffer error\n");
		return -1;
	}

	/* get parameter value from config.ini */
	value_recver[0] = 0;
	ret = tr069_get_config(param->param_index, value_recver, value_size);

	if (0 == ret) {
		tr069_debug("ok quit value = %s\n", value_recver);
		return 0;
	} else {
		tr069_error("tr069_get_config error %s\n", param->param_index);
		return -1;
	}
}

INT32_T tr069_parameter_get_attribute(tr069_parameter_mgr *param_handle, tr069_parameter *param, INT32_T *value_recver, INT32_T value_size)
{
	tr069_debug("enter\n");
	if (NULL == param_handle) {
		tr069_error("parameter handle error\n");
		return -1;
	}

	if (NULL == param) {
		tr069_error("parameter error\n");
		return -1;
	}

	if (NULL == value_recver) {
		tr069_error("value receive buffer error\n");
		return -1;
	}

	*value_recver = param->notif_type;

	return FV_OK;

}

const INT8_T* tr069_data_type_name(tr069_data_type data_type)
{
	const INT8_T *data_type_name = NULL;

	switch (data_type) {
	case TR069_DATATYPE_OBJECT:
		data_type_name = "object";
		break;

	case TR069_DATATYPE_STRING:
		data_type_name = "string";
		break;

	case TR069_DATATYPE_INT:
		data_type_name = "int";
		break;

	case TR069_DATATYPE_UNSIGNEDINT:
		data_type_name = "unsignedInt";
		break;

	case TR069_DATATYPE_BOOLEAN:
		data_type_name = "boolean";
		break;

	case TR069_DATATYPE_DATETIME:
		data_type_name = "dateTime";
		break;

	case TR069_DATATYPE_BASE64:
		data_type_name = "base64";
		break;

	default:
		break;
	}

	return data_type_name;
}


