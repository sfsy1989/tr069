/********************************************************************
	Created By Ash, 2014/3/25
	Model: Tr069
	Sub Model: Tr111 Control
	Function: Tr111 task control and execute
********************************************************************/
#include <n_lib.h>
#include <time.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>

#include "tr069.h"
#include "tr111_control.h"
#include "tr069_service.h"

/* global */

/* extern */
extern int tr069_set_config(int key, char *value);
extern int tr069_get_config(int key, char *value, unsigned int size);

extern tr069_mgr *g_tr069;
extern int g_tr069_stand;
extern char* tr069_parameter_name_table[TR069_PARAM_NUM];
/* local */
int g_tr111_timer;

tr111_control_mgr* tr111_control_init(tr069_mgr *tr069_handle)
{
	tr111_control_mgr *manager = NULL;

	tr069_debug("enter\n");
	if (NULL == tr069_handle) {
		tr069_error("tr069 handle error\n");
		goto ERROR_EXIT;
	}

	/* initialize control manager */
	manager = malloc(sizeof(tr111_control_mgr));
	if (NULL == manager) {
		tr069_error("malloc control manager error\n");
		goto ERROR_EXIT;
	}
	memset(manager, 0, sizeof(tr111_control_mgr));

//	 struct sockaddr_in server_addr;   
//	 bzero(&server_addr, sizeof(server_addr));   
//	 server_addr.sin_family = AF_INET;   
//	 server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
//	 server_addr.sin_port = htons(3798);      
	 /* ´´½¨socket */  
//	 int client_socket_fd = socket(AF_INET, SOCK_DGRAM, 0); 
//	 bind(client_socket_fd,(struct sockaddr *)&server_addr,sizeof(server_addr));
	 /* only initialize basic elements */
	manager->tr069_handle = tr069_handle;
	manager->status = TR111_CONTROL_STATUS_STOP;
	manager->sockfd = -1;

	tr069_debug("ok quit\n");
	return manager;

ERROR_EXIT:

	/* if control manger already initialized, uninitialize it */
	if (manager != NULL) {
		manager = tr111_control_uninit(manager);
	}

	tr069_error("error quit\n");
	return NULL;
}

tr111_control_mgr* tr111_control_uninit(tr111_control_mgr *control_handle)
{
	tr069_debug("enter\n");

	if (control_handle->sockfd >= 0) {
		shutdown(control_handle->sockfd, SHUT_RDWR);
		close(control_handle->sockfd);
		control_handle->sockfd = 0;
	}

	if (control_handle != NULL) {
		free(control_handle);
	}

	tr069_debug("ok quit\n");
	return NULL;
}

static int tr111_apply_new_address(tr111_control_mgr *control_handle, struct sockaddr_in new_addr)
{
	char temp_string[TR069_MAX_PARAM_VALUE_LEN] = {0};
	char inform_flag = 0;

	tr069_debug("enter\n");
	if (control_handle == NULL) {
		tr069_error("control handle error\n");
		return -1;
	}

	/* check for change */
	if (control_handle->public_addr.sin_port != new_addr.sin_port ||
		control_handle->public_addr.sin_addr.s_addr != new_addr.sin_addr.s_addr) {
		tr069_debug("udp connection request address change\n");
		snprintf(temp_string, sizeof(temp_string), "%s:%d",
			inet_ntoa(new_addr.sin_addr.s_addr),
			ntohs(new_addr.sin_port));
		tr069_set_config(TR069_PARAM_UDPConnectionRequestAddress, temp_string);
		tr069_debug("udp connection request address=%s\n", temp_string);
		tr069_service_put_inform_parameter(PCODE_UDPConnectionRequestAddress);
		inform_flag = 1;
		/* send binding change immediately */
		/* maybe we should use tr111_control_check_stun_params */
		tr069_get_config(TR069_PARAM_STUNEnable, temp_string, sizeof(temp_string));
		if (strcmp(temp_string, "true") == 0) {
			control_handle->status = TR111_CONTROL_STATUS_TASK;
			control_handle->retry_count = 0;
			control_handle->binding_change_flag = 1;
		}
	}

	/* check for nat */
	if (control_handle->local_addr.sin_port != new_addr.sin_port ||
		control_handle->local_addr.sin_addr.s_addr != new_addr.sin_addr.s_addr) {
		tr069_debug("nat true\n");
		if (control_handle->local_addr.sin_port == control_handle->public_addr.sin_port &&
			control_handle->local_addr.sin_addr.s_addr == control_handle->public_addr.sin_addr.s_addr) {
			tr069_debug("nat change\n");
			tr069_set_config(TR069_PARAM_NATDetected, "true");
			tr069_service_put_inform_parameter(PCODE_NATDetected);
			inform_flag = 1;
		}
	}
	else {
		tr069_debug("nat false\n");
		if (control_handle->local_addr.sin_port != control_handle->public_addr.sin_port ||
			control_handle->local_addr.sin_addr.s_addr != control_handle->public_addr.sin_addr.s_addr) {
			tr069_debug("nat change\n");
			tr069_set_config(TR069_PARAM_NATDetected, "false");
			tr069_service_put_inform_parameter(PCODE_NATDetected);
			inform_flag = 1;
		}
	}

	control_handle->public_addr.sin_port = new_addr.sin_port;
	control_handle->public_addr.sin_addr.s_addr = new_addr.sin_addr.s_addr;

	if (inform_flag) {
		tr069_debug("trigger inform\n");
		tr069_service_set_event(TR069_EVENT_VALUE_CHANGE, 1, "");
		tr069_service_start();
	}

	return FV_OK;
}

static int tr111_control_parse_udp_connection_request(tr111_control_mgr *control_handle)
{
	char key[TR069_MAX_PARAM_VALUE_LEN] = {0};
	char text[TR069_MAX_PARAM_VALUE_LEN] = {0};
	char this_ts[TR111_STUN_ATTR_MAX_SIZE] = {0};
	char this_id[TR111_STUN_ATTR_MAX_SIZE] = {0};
	static char last_ts[TR111_STUN_ATTR_MAX_SIZE] = {0};
	static char last_id[TR111_STUN_ATTR_MAX_SIZE] = {0};
	char recv_hmac[TR111_STUN_ATTR_MAX_SIZE] = {0};
	char calc_hmac[TR111_STUN_ATTR_MAX_SIZE] = {0};
	char hex_string[3] = {0};
	unsigned int hmac_len, i;
	char *puri = NULL;
	char *ptemp = NULL;
	char *pend = NULL;

	tr069_debug("enter\n");
	if (control_handle == NULL) {
		tr069_error("control handle error\n");
		return -1;
	}

	puri = control_handle->recv_buff;

	/* check for method */
	if (memcmp(puri, "GET ", strlen("GET ")) != 0) {
		tr069_error("method error\n");
		return -1;
	}
	puri += strlen("GET ");

	/* search for uri end */
	ptemp = strstr(puri, " ");
	if (ptemp == NULL) {
		tr069_error("space error\n");
		return -2;
	}
	*ptemp = 0;

	/* get key */
	tr069_get_config(TR069_PARAM_ConnectionRequestPassword, key, sizeof(key));

	/* get text */
	memset(text, 0, TR069_MAX_PARAM_VALUE_LEN);
	ptemp = strstr(puri, "ts=");
	if (ptemp == NULL) {
		tr069_error("ts error\n");
		return -3;
	}
	ptemp += strlen("ts=");
	pend = strstr(ptemp, "&");
	if (pend == NULL) {
		pend = puri + strlen(puri);
	}
	memcpy(text + strlen(text), ptemp, pend - ptemp);
	memcpy(this_ts, ptemp, pend - ptemp);
	this_ts[pend - ptemp] = 0;
//	if (atoi(this_ts) <= atoi(last_ts)) {
//		tr069_error("ts error\n");
//		return -3;
//	}
	ptemp = strstr(puri, "id=");
	if (ptemp == NULL) {
		tr069_error("id error\n");
		return -4;
	}
	ptemp += strlen("id=");
	pend = strstr(ptemp, "&");
	if (pend == NULL) {
		pend = puri + strlen(puri);
	}
	memcpy(text + strlen(text), ptemp, pend - ptemp);
	memcpy(this_id, ptemp, pend - ptemp);
	this_id[pend - ptemp] = 0;
	if (strcmp(this_id, last_id) == 0) {
		tr069_error("id error\n");
		return -4;
	}
	ptemp = strstr(puri, "un=");
	if (ptemp == NULL) {
		tr069_error("un error\n");
		return -5;
	}
	ptemp += strlen("un=");
	pend = strstr(ptemp, "&");
	if (pend == NULL) {
		pend = puri + strlen(puri);
	}
	memcpy(text + strlen(text), ptemp, pend - ptemp);
	ptemp = strstr(puri, "cn=");
	if (ptemp == NULL) {
		tr069_error("cn error\n");
		return -6;
	}
	ptemp += strlen("cn=");
	pend = strstr(ptemp, "&");
	if (pend == NULL) {
		pend = puri + strlen(puri);
	}
	memcpy(text + strlen(text), ptemp, pend - ptemp);
	/* get sig */
	ptemp = strstr(puri, "sig=");
	if (ptemp == NULL) {
		tr069_error("sig error\n");
		return -7;
	}
	ptemp += strlen("sig=");
	pend = strstr(ptemp, "&");
	if (pend == NULL) {
		pend = puri + strlen(puri);
	}
	memcpy(recv_hmac, ptemp, pend - ptemp);
	recv_hmac[pend - ptemp] = 0;

	/* calc hmac */
	HMAC(EVP_sha1(), key, strlen(key), text, strlen(text), calc_hmac, &hmac_len);
	if (hmac_len != TR111_MESSAGEINTEGRITY_LENGTH) {
		tr069_error("hmac length error\n");
		return -8;
	}

	memset(hex_string, 0, sizeof(hex_string));
	for (i = 0; i < TR111_MESSAGEINTEGRITY_LENGTH; i++) {
		memcpy(hex_string, 2 * i + recv_hmac, 2);
		if (strtol(hex_string, NULL, 16) != calc_hmac[i]) {
			tr069_error("signature error\n");
			return -9;
		}
	}

	/* trigger connection request */
	tr069_error("a valid udp connection request\n");
	tr069_service_set_event(TR069_EVENT_CONNECTION_REQUEST, 1, "");
	tr069_service_put_inform_parameter(PCODE_STBID);
	tr069_service_put_inform_parameter(PCODE_IPAddressReal);
	tr069_service_start();

	strcpy(last_ts, this_ts);
	strcpy(last_id, this_id);

	return FV_OK;
}

static int tr111_control_parse_message(tr111_control_mgr *control_handle)
{
	unsigned char recv_integrity[TR111_STUN_ATTR_MAX_SIZE];
	unsigned int hmac_len = 0;
	unsigned char *pattr;
	int byte_left;
	msg_header mheader;
	attr_header aheader;
	mapped_address maddr;
	struct sockaddr_in new_addr;
	error_code ecode;
	char maddr_flag = 0;
	char integrity_flag = 0;

	tr069_debug("enter\n");
	if (control_handle == NULL) {
		tr069_error("control handle error\n");
		return -1;
	}

	if (control_handle->status != TR111_CONTROL_STATUS_TASK) {
		tr069_error("not in task status\n");
		return -1;
	}

	byte_left = control_handle->recv_len;
	pattr = control_handle->recv_buff;

	/* parse message header */
	if (byte_left < sizeof(mheader)) {
		tr069_error("parse message header error\n");
		return -2;
	}
	memcpy(&mheader, pattr, sizeof(mheader));
	mheader.type = ntohs(mheader.type);
	mheader.length = ntohs(mheader.length);
	byte_left -= sizeof(mheader);
	pattr += sizeof(mheader);
	/* check message length */
	if (control_handle->recv_len != sizeof(mheader) + mheader.length) {
		tr069_error("message header length error\n");
		return -3;
	}

	/* check transaction id */
	if (memcmp(mheader.trans_id, control_handle->trans_id, sizeof(mheader.trans_id)) != 0) {
		tr069_error("transaction id error\n");
		return -4;
	}

	switch (mheader.type) {
	case TR111_MSGTYPE_BINDING_RESPONSE:
	case TR111_MSGTYPE_BINDING_ERROR_RESPONSE:
		tr069_debug("receive message type=%d\n", mheader.type);
		break;

	default:
		tr069_error("unsupport message type\n");
		return -5;
	}

	while (byte_left > 0) {
		/* parse attribute header */
		if (byte_left < sizeof(aheader)) {
			tr069_error("attribute header length error\n");
			return -6;
		}
		memcpy(&aheader, pattr, sizeof(aheader));
		aheader.type = ntohs(aheader.type);
		aheader.length = ntohs(aheader.length);
		byte_left -= sizeof(aheader);
		pattr += sizeof(aheader);

		/* parse attribute value */
		if (byte_left < aheader.length) {
			tr069_error("attribute value length error\n");
			return -7;
		}
		switch (aheader.type) {
		case TR111_ATTRTYPE_MAPPEDADDRESS:
			if (mheader.type != TR111_MSGTYPE_BINDING_RESPONSE) {
				tr069_error("mapped address can only appear in binding response\n");
				return -8;
			}
			/* mapped address value length is fixed */
			if (aheader.length != sizeof(maddr)) {
				tr069_error("mapped address length error\n");
				return -9;
			}
			memcpy(&maddr, pattr, aheader.length);
			/* check family */
			if (maddr.family != 0x01) {
				tr069_error("family not support\n");
				return -10;
			}
			/* sockaddr use net order */
			new_addr.sin_port = maddr.port;
			new_addr.sin_addr.s_addr = maddr.address;
			maddr_flag = 1;
			break;

		case TR111_ATTRTYPE_MESSAGEINTEGRITY:
			if (mheader.type != TR111_MSGTYPE_BINDING_RESPONSE) {
				tr069_error("message integrity can only appear in binding response\n");
				return -11;
			}
			if (aheader.length != TR111_MESSAGEINTEGRITY_LENGTH ||
				byte_left != TR111_MESSAGEINTEGRITY_LENGTH) {
				tr069_error("message integrity must be last 20 bytes of message\n");
				return -12;
			}
			/* calcuate integrity and compare */
			// todo:password here ?
			HMAC(EVP_sha1(), control_handle->stun_password, strlen(control_handle->stun_password),
				control_handle->recv_buff, pattr - control_handle->recv_buff - sizeof(aheader), recv_integrity, &hmac_len);
			if (hmac_len != TR111_MESSAGEINTEGRITY_LENGTH) {
				tr069_error("hmac length error\n");
				return -13;
			}
			if (memcmp(recv_integrity, pattr, TR111_MESSAGEINTEGRITY_LENGTH) != 0) {
				tr069_error("integrity error\n");
				return -14;
			}
			integrity_flag = 1;
			break;

		case TR111_ATTRTYPE_ERRORCODE:
			if (mheader.type != TR111_MSGTYPE_BINDING_ERROR_RESPONSE) {
				tr069_error("errorcode can only appear in binding error response\n");
				return -15;
			}
			if (aheader.length < sizeof(ecode)) {
				tr069_error("error code length error\n");
				return -16;
			}
			memcpy(&ecode, pattr, sizeof(ecode));
			if ((ecode.code_1 * 100 + ecode.code_2) == TR111_STUN_ERROR_UNAUTHORIZED) {
				tr069_debug("401 need authentication\n");
				control_handle->auth_flag = 1;
				/* resend auth request */
				control_handle->status = TR111_CONTROL_STATUS_TASK;
				control_handle->retry_count = 0;
			}
			else {
				tr069_debug("unsupport error code\n");
			}
			break;

		default:
			tr069_debug("ignore attribute type=%d\n", aheader.type);
		}

		byte_left -= aheader.length;
		pattr += aheader.length;
	}

	/* check mapped address there to make sure only valid integrity */
	if (mheader.type == TR111_MSGTYPE_BINDING_RESPONSE) {
		if (control_handle->auth_flag == 1 && integrity_flag == 0) {
			tr069_error("expected integrity is not present\n");
			return -17;
		}
		if (maddr_flag) {
			tr111_apply_new_address(control_handle, new_addr);
		}
		return FV_OK;
	}
	else {
		return -18;
	}
}

static int tr111_control_new_transaction(tr111_control_mgr *control_handle)
{
	tr069_debug("enter\n");
	if (control_handle == NULL) {
		tr069_error("control handle error\n");
		return -1;
	}

	/* clear flags and trans id */
	control_handle->retry_count = 0;
	control_handle->auth_flag = 0;
	control_handle->binding_change_flag = 0;
	srand(time(NULL));
	control_handle->trans_id[0] = rand();
	control_handle->trans_id[1] = rand();
	control_handle->trans_id[2] = rand();
	control_handle->trans_id[3] = rand();

	return FV_OK;
}

static int tr111_control_setup(tr111_control_mgr *control_handle)
{
	char temp_string[TR069_MAX_PARAM_VALUE_LEN] = {0};

	tr069_debug("enter\n");
	if (control_handle == NULL) {
		tr069_error("control handle error\n");
		return -1;
	}

	/* setup udp connect request address */
	tr069_get_config(TR069_PARAM_IPAddressReal, temp_string, sizeof(temp_string));
	snprintf(temp_string, sizeof(temp_string), "%s:%d", temp_string, TR111_UDP_CONNECTION_REQUEST_PORT);
	tr069_set_config(TR069_PARAM_UDPConnectionRequestAddress, temp_string);
	tr069_error("udp connection request address=%s\n", temp_string);

	/* setup socket */
	if (control_handle->sockfd >= 0) {
		tr069_debug("close previous socket first\n");
		shutdown(control_handle->sockfd, SHUT_RDWR);
		close(control_handle->sockfd);
		control_handle->sockfd = 0;
	}
	control_handle->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (control_handle->sockfd < 0) {
		tr069_error("create socket error\n");
		return -1;
	}
	memset(&(control_handle->local_addr), 0, sizeof(control_handle->local_addr));
	control_handle->local_addr.sin_family = AF_INET;
	control_handle->local_addr.sin_port = htons(TR111_UDP_CONNECTION_REQUEST_PORT);
	tr069_get_config(TR069_PARAM_IPAddressReal, temp_string, sizeof(temp_string));
	control_handle->local_addr.sin_addr.s_addr = inet_addr(temp_string);
	if (bind(control_handle->sockfd, (struct sockaddr*)&(control_handle->local_addr), sizeof(control_handle->local_addr)) < 0) {
		tr069_error("bind socket error\n");
		return -1;
	}

	/* default public = local */
	control_handle->public_addr.sin_family = AF_INET;
	control_handle->public_addr.sin_port = control_handle->local_addr.sin_port;
	control_handle->public_addr.sin_addr.s_addr = control_handle->local_addr.sin_addr.s_addr;

	/* default no nat */
	tr069_set_config(TR069_PARAM_NATDetected, "false");

	return FV_OK;
}

static int tr111_control_check_stun_params(tr111_control_mgr *control_handle)
{
	char temp_string[TR069_MAX_PARAM_VALUE_LEN] = {0};

	tr069_debug("enter\n");
	if (control_handle == NULL) {
		tr069_error("control handle error\n");
		return -1;
	}

	/* check if stun enabled */
	tr069_get_config(TR069_PARAM_STUNEnable, temp_string, sizeof(temp_string));
	if (strcmp(temp_string, "true")) {
		tr069_debug("stun disabled\n");
		return -2;
	}

	/* check stun address */
	memset(&(control_handle->stun_addr), 0, sizeof(control_handle->stun_addr));
	control_handle->stun_addr.sin_family = AF_INET;
	//tr069_get_config(TR069_PARAM_STUNServerAddress, temp_string, sizeof(temp_string));
	//if (temp_string[0] == 0) {
	//	tr069_debug("stun server not specified, use acs instead\n");
	//	tr069_get_config(TR069_PARAM_ManageSerURL, temp_string, sizeof(temp_string));
	//}	
	sprintf(temp_string,"%s","172.16.199.18");	
	control_handle->stun_addr.sin_addr.s_addr = inet_addr(temp_string);
	
	//tr069_get_config(TR069_PARAM_STUNServerPort, temp_string, sizeof(temp_string));
	sprintf(temp_string,"%s","3478");	
	if (atoi(temp_string) <= 0) {
		tr069_error("specified stun port error\n");
		return -3;
	}
	control_handle->stun_addr.sin_port = htons(atoi(temp_string));

	/* now use static period, future use timeout discovery */
	tr069_get_config(TR069_PARAM_STUNMaxKeepAlivePeriod, temp_string, sizeof(temp_string));
	control_handle->max_keepalive_period = atoi(temp_string);
	tr069_get_config(TR069_PARAM_STUNMinKeepAlivePeriod, temp_string, sizeof(temp_string));
	control_handle->min_keepalive_period = atoi(temp_string);
	if (control_handle->max_keepalive_period == 0 &&
		control_handle->min_keepalive_period == 0) {
		tr069_debug("undefined max min keepalive period\n");
		control_handle->keepalive_period = 10;
	}
	else if (control_handle->min_keepalive_period > control_handle->max_keepalive_period) {
		tr069_debug("keepalive min < max\n");
		control_handle->keepalive_period = control_handle->min_keepalive_period;
	}
	else {
		control_handle->keepalive_period = (control_handle->max_keepalive_period + control_handle->min_keepalive_period) / 2;
	}

	/* check stun username and password */
	//tr069_get_config(TR069_PARAM_STUNUsername, control_handle->stun_username, F_MAX_USERNAME_SIZE);
	__system_property_get("ro.fonsview.stb.STBID",control_handle->stun_username);
	//sprintf(control_handle->stun_username,"%s","stbId");
	tr069_get_config(TR069_PARAM_STUNPassword, control_handle->stun_password, F_MAX_PASSWORD_SIZE);
	//sprintf(control_handle->stun_password,"%s","STBAdmin");
	//tr069_error("[xiaoniu]stun password=%s\n", control_handle->stun_password);

	return FV_OK;
}

static int tr111_control_recvfrom(tr111_control_mgr *control_handle)
{
	int ret;

	tr069_debug("enter\n");
	if (control_handle == NULL) {
		tr069_error("control handle error\n");
		return -1;
	}
	
	struct timeval tv_out;
     tv_out.tv_sec = 5;//µÈ´ý5Ãë
     tv_out.tv_usec = 0;
	 setsockopt(control_handle->sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));
	
	control_handle->recv_len = recvfrom(control_handle->sockfd, control_handle->recv_buff,
		sizeof(control_handle->recv_buff), 0, 0, 0);
	if (control_handle->recv_len > 0 && 
		control_handle->recv_len <= TR111_STUN_MSG_MAX_SIZE) {
		/* got some packets */
		if (control_handle->recv_buff[0] == 0 || control_handle->recv_buff[0] == 1) {
			/* seem to be stun message */
			ret = tr111_control_parse_message(control_handle);
			if (ret == FV_OK) {
				control_handle->status = TR111_CONTROL_STATUS_WAIT;
				control_handle->wait_time = time(NULL) + control_handle->keepalive_period;
			}
		}
		else if (control_handle->recv_buff[0] >= 32 &&
			control_handle->recv_buff[0] <= 128) {
			/* seem to be udp connection request */
			ret = tr111_control_parse_udp_connection_request(control_handle);
		}
		else {
			/* error packet */
			tr069_error("unsupport packet\n");
			return -2;
		}
	}

	return ret;
}

static int tr111_control_sendto(tr111_control_mgr *control_handle)
{
	tr069_debug("enter\n");
	if (control_handle == NULL) {
		tr069_error("control handle error\n");
		return -1;
	}

	sendto(control_handle->sockfd, control_handle->send_buff, control_handle->send_len, 0,
		(struct sockaddr*)&(control_handle->stun_addr), sizeof(struct sockaddr));
	tr069_error("sendto length=%d\n", control_handle->send_len);

	return FV_OK;
}

static int tr111_control_build_message(tr111_control_mgr *control_handle, tr111_message_type type)
{
	unsigned char *pattr;
	unsigned int hmac_len;
	msg_header mheader;
	attr_header aheader;
	
	tr069_debug("enter\n");
	if (control_handle == NULL) {
		tr069_error("control handle error\n");
		return -1;
	}

	switch (type) {
	case TR111_MSGTYPE_BINDING_REQUEST:
		pattr = control_handle->send_buff;
		/* set message header */
		mheader.type = htons(type);
		mheader.length = 0;
		memcpy(mheader.trans_id, control_handle->trans_id, sizeof(mheader.trans_id));
		pattr += sizeof(mheader);
		/* set attributes if needed */
		/* set connection request binding */
		aheader.type = htons(TR111_ATTRTYPE_CONNECTIONREQUESTBINDING);
		aheader.length = htons(TR111_CONNECTIONREQUESTBINDING_LENGTH);
		memcpy(pattr, &aheader, sizeof(aheader));
		pattr += sizeof(aheader);
		mheader.length += sizeof(aheader);
		memcpy(pattr, TR111_CONNECTIONREQUESTBINDING_VALUE, TR111_CONNECTIONREQUESTBINDING_LENGTH);
		pattr += TR111_CONNECTIONREQUESTBINDING_LENGTH;
		mheader.length += TR111_CONNECTIONREQUESTBINDING_LENGTH;
		/* set binding change */
		if (control_handle->binding_change_flag) {
			aheader.type = htons(TR111_ATTRTYPE_BINDINGCHANGE);
			aheader.length = 0;
			memcpy(pattr, &aheader, sizeof(aheader));
			pattr += sizeof(aheader);
			mheader.length += sizeof(aheader);
		}
		/* set username */
		if (control_handle->stun_username[0]) {
			unsigned char ulen = strlen(control_handle->stun_username);
			ulen += (4 - (strlen(control_handle->stun_username) % 4)) % 4;
			aheader.type = htons(TR111_ATTRTYPE_USERNAME);
			aheader.length = htons(ulen);
			memcpy(pattr, &aheader, sizeof(aheader));
			pattr += sizeof(aheader);
			mheader.length += sizeof(aheader);
			memset(pattr, 0, ulen);
			memcpy(pattr, control_handle->stun_username, strlen(control_handle->stun_username));
			pattr += ulen;
			mheader.length += ulen;
		}
		/* set message integrity, first fill zero */
		if (control_handle->auth_flag && control_handle->stun_password[0]) {			
			aheader.type = htons(TR111_ATTRTYPE_MESSAGEINTEGRITY);
			aheader.length = htons(TR111_MESSAGEINTEGRITY_LENGTH);
			memcpy(pattr, &aheader, sizeof(aheader));
			pattr += sizeof(aheader);
			mheader.length += sizeof(aheader);
			memset(pattr, 0, TR111_MESSAGEINTEGRITY_LENGTH);
			//pattr += TR111_MESSAGEINTEGRITY_LENGTH;
			mheader.length += TR111_MESSAGEINTEGRITY_LENGTH;
		}
		/* complete message header */
		control_handle->send_len = mheader.length + sizeof(mheader);
		mheader.length = htons(mheader.length);
		memcpy(control_handle->send_buff, &mheader, sizeof(mheader));
		/* complete message integrity */
		if (control_handle->auth_flag && control_handle->stun_password[0]) {
			HMAC(EVP_sha1(), control_handle->stun_password, strlen(control_handle->stun_password),
				control_handle->send_buff, pattr - control_handle->send_buff - sizeof(aheader), pattr, &hmac_len);
			if (hmac_len != TR111_MESSAGEINTEGRITY_LENGTH) {
				tr069_error("hmac length error\n");
				return -1;
			}
		}
		break;

	default:
		tr069_debug("unsupport message type\n");
		return -1;
	}

	return FV_OK;
}

static int tr111_control_task_proc(void *arg)
{
	static int ret;
	static unsigned int last_time = 0;
	static unsigned int now_time = 0;
	static tr111_control_mgr *control_handle;
	static tr069_mgr *tr069_handle = NULL;

	control_handle = (tr111_control_mgr *)arg;
	if (control_handle == NULL) {
		tr069_error("[tr0111] control handle error\n");
		goto FINISH_EXIT;
	}

	tr069_handle = control_handle->tr069_handle;
	if (NULL == tr069_handle) {
		tr069_error("[tr0111] tr069 handle error\n");
		goto FINISH_EXIT;
	}

	/* receive udp packet, maybe udp connection request or stun response */
	tr111_control_recvfrom(control_handle);

	now_time = time(NULL);
	switch (control_handle->status) {
	case TR111_CONTROL_STATUS_IDLE:
		ret = tr111_control_check_stun_params(control_handle);
		if (ret == FV_OK) {
			/* stun parameters ok, start stun task */
			tr111_control_new_transaction(control_handle);
			tr111_control_setup(control_handle);
			control_handle->status = TR111_CONTROL_STATUS_TASK;
		}
		break;

	case TR111_CONTROL_STATUS_TASK:
		/* check and update stun parameters */
		ret = tr111_control_check_stun_params(control_handle);
		if (ret != FV_OK) {
			/* stun parameters error, back to idle */
			control_handle->status = TR111_CONTROL_STATUS_IDLE;
			tr111_apply_new_address(control_handle, control_handle->local_addr);
			break;
		}
		/* check retry policy */
		if ((last_time != 0  && (now_time - last_time) > TR111_MSG_TIMEOUT_SEC) ||
			control_handle->retry_count == 0) {
			if (control_handle->retry_count > TR111_MSG_RETRY_TIMES) {
				/* retry too many times, start new transaction */
				tr111_control_new_transaction(control_handle);
				break;
			}
			/* build message to send */
			ret = tr111_control_build_message(control_handle, TR111_MSGTYPE_BINDING_REQUEST);
			if (ret == FV_OK) {
				/* stun message built, now send it */
				tr111_control_sendto(control_handle);
				last_time = now_time;
				(control_handle->retry_count)++;
			}
		}
		break;

	case TR111_CONTROL_STATUS_WAIT:
		if (now_time >= control_handle->wait_time) {
			tr069_error("[tr0111] wait enough, back to task\n");
			tr111_control_new_transaction(control_handle);
			control_handle->status = TR111_CONTROL_STATUS_TASK;
		}
		break;
		
	case TR111_CONTROL_STATUS_STOP:
	default:
		/* somebody ask to stop */
		tr069_error("[tr0111] execute stop\n");
		goto FINISH_EXIT;
	}

	return FV_OK;

FINISH_EXIT:
	tr069_debug("[tr0111] ok quit\n");

	return -1;
}

int tr111_control_main_task(tr111_control_mgr *control_handle)
{
	tr069_debug("enter\n");

	if (NULL == control_handle) {
		tr069_error("control handle error\n");
		return -1;
	}

	control_handle->status = TR111_CONTROL_STATUS_IDLE;
	g_tr111_timer = f_timer_add(5000, tr111_control_task_proc, (void *)control_handle);
	if (g_tr111_timer <= 0) {
		tr069_error("[tr0111] f_timer_add failed.\n");
		return -1;
	}

	return FV_OK;
}

