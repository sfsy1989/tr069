#include "curl/curl.h"

char send_buff1[] = "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:cwmp=\"urn:dslforum-org:cwmp-1-0\" xmlns:xsi=\"http://www.w3.org/1999/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/1999/XMLSchema\"><soap:Body><cwmp:Inform><DeviceId xsi:type=\"cwmp:DeviceIdStruct\"><Manufacturer>SomeBrand</Manufacturer><OUI>990002</OUI><ProductClass>Class1</ProductClass><SerialNumber>000000000001</SerialNumber></DeviceId><Event soap:arrayType=\"cwmp:EventStruct[1]\"><EventStruct><EventCode>1 BOOT</EventCode><CommandKey></CommandKey></EventStruct></Event><MaxEnvelopes>1</MaxEnvelopes><CurrentTime>Thu Jan  1 08:05:49 1970</CurrentTime><RetryCount>0</RetryCount><ParameterList soap:arrayType=\"cwmp:ParameterValueStruct[6]\"><ParameterValueStruct><Name>Device.DeviceInfo.HardwareVersion</Name><Value>1.5.2</Value></ParameterValueStruct><ParameterValueStruct><Name>Device.DeviceInfo.SoftwareVersion</Name><Value>1.5.2</Value></ParameterValueStruct><ParameterValueStruct><Name>Device.ManagementServer.ConnectionRequestURL</Name><Value>http://172.16.200.149/tr069/tr069.cgi</Value></ParameterValueStruct><ParameterValueStruct><Name>Device.X_CTC_IPTV.STBID</Name><Value>00100199000270100001000763bc614e</Value></ParameterValueStruct><ParameterValueStruct><Name>Device.X_CTC_IPTV.ServiceInfo.UserID</Name><Value>1</Value></ParameterValueStruct><ParameterValueStruct><Name>Device.DeviceInfo.ProvisioningCode</Name><Value>SomeCode</Value></ParameterValueStruct></ParameterList></cwmp:Inform></soap:Body></soap:Envelope>";
char send_buff2[] = "";

int main(int argc, char **argv)
{
	void *curl_handle;

	if (argc != 2)
	{
		printf("argument error\n");
		return -1;
	}

	printf("init\n");
	curl_global_init(CURL_GLOBAL_WIN32);
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30);

	printf("send inform\n");
	curl_easy_setopt(curl_handle, CURLOPT_URL, argv[1]);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, send_buff1);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)strlen(send_buff1));
	curl_easy_perform(curl_handle);

	printf("send empty\n");
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, send_buff2);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)strlen(send_buff2));
	curl_easy_perform(curl_handle);

	printf("quit\n");
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();

	return 0;
}
