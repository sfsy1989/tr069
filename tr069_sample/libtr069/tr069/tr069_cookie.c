#include <n_lib.h>
#include <curl/curl.h>
#include "tr069_interface.h"
#include "tr069_rpc.h"
#include "tr069.h"

int tr069_set_req_cookie(tr069_rpc_mgr *rpc_handle)
{
	tr069_debug("\n");

	static f_stb_status_s *stb_stat = NULL;

	if (NULL == stb_stat) {
		stb_stat = n_get_stb_stat();
	}

	tr069_interface_mgr *interface_handle = NULL;
	VOID_T *curl_handle = NULL;
	interface_handle = rpc_handle->tr069_handle->interface_handle;
	f_assert(NULL != interface_handle);
	curl_handle = interface_handle->curl_handle;
	f_assert(NULL != curl_handle);

	if ('\0' != stb_stat->tr069_session[0]) {
		curl_easy_setopt(curl_handle, CURLOPT_COOKIE, stb_stat->tr069_session);
		stb_stat->tr069_session[0] = '\0';
	}

	return FV_OK;
}

int tr069_clr_req_cookie(tr069_rpc_mgr *rpc_handle)
{
	tr069_debug("\n");

	tr069_interface_mgr *interface_handle = NULL;
	VOID_T *curl_handle = NULL;
	interface_handle = rpc_handle->tr069_handle->interface_handle;
	f_assert(NULL != interface_handle);
	curl_handle = interface_handle->curl_handle;
	f_assert(NULL != curl_handle);

	curl_easy_setopt(curl_handle, CURLOPT_COOKIE, "");

	return FV_OK;
}

int tr069_clear_cookie(tr069_rpc_mgr *rpc_handle)
{
	tr069_debug("\n");

	tr069_interface_mgr *interface_handle = NULL;
	VOID_T *curl_handle = NULL;
	interface_handle = rpc_handle->tr069_handle->interface_handle;
	f_assert(NULL != interface_handle);
	curl_handle = interface_handle->curl_handle;
	f_assert(NULL != curl_handle);

	curl_easy_setopt(curl_handle, CURLOPT_COOKIESESSION, 1);
	curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, "ALL");
	curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, "FLUSH");
	return FV_OK;
}

int tr069_load_cookie(tr069_rpc_mgr *rpc_handle)
{
	tr069_debug("\n");

	tr069_interface_mgr *interface_handle = NULL;
	VOID_T *curl_handle = NULL;
	interface_handle = rpc_handle->tr069_handle->interface_handle;
	f_assert(NULL != interface_handle);
	curl_handle = interface_handle->curl_handle;
	f_assert(NULL != curl_handle);

	curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, N_STB_PARAM_PATH "/cookie.txt");
	return FV_OK;
}

int tr069_save_cookie(tr069_rpc_mgr *rpc_handle)
{
	tr069_debug("\n");

	tr069_interface_mgr *interface_handle = NULL;
	VOID_T *curl_handle = NULL;
	interface_handle = rpc_handle->tr069_handle->interface_handle;
	f_assert(NULL != interface_handle);
	curl_handle = interface_handle->curl_handle;
	f_assert(NULL != curl_handle);

	curl_easy_setopt(curl_handle, CURLOPT_COOKIEJAR, N_STB_PARAM_PATH "/cookie.txt");
	curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, "FLUSH");

	return FV_OK;
}

