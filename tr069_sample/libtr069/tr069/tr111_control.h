/********************************************************************
	Created By Ash, 2014/3/25
	Model: Tr069
	Sub Model: Tr111 Control
	Function: Tr111 task control and execute
********************************************************************/
#ifndef _TR111_CONTROL_
#define _TR111_CONTROL_

#include <netinet/in.h>
#include <n_stb_tr069.h>
#include "tr069_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TR111_MSG_TIMEOUT_SEC 3
#define TR111_MSG_RETRY_TIMES 2
#define TR111_UDP_CONNECTION_REQUEST_PORT 8090
#define TR111_STUN_MSG_HEADER_SIZE 20
#define TR111_STUN_MSG_MAX_SIZE 1024
#define TR111_STUN_ATTR_HEADER_SIZE 8
#define TR111_STUN_ATTR_MAX_SIZE 128
#define TR111_STUN_ERROR_UNAUTHORIZED 401

#define TR111_CONNECTIONREQUESTBINDING_LENGTH 20
#define TR111_CONNECTIONREQUESTBINDING_VALUE "\x64\x73\x6C\x66\x6F\x72\x75\x6D\x2E\x6F\x72\x67\x2F\x54\x52\x2D\x31\x31\x31\x20"
#define TR111_MESSAGEINTEGRITY_LENGTH 20

/* stun message type concerned */
typedef enum
{
	TR111_MSGTYPE_BINDING_REQUEST = 0x0001,
	TR111_MSGTYPE_BINDING_RESPONSE = 0x0101,
	TR111_MSGTYPE_BINDING_ERROR_RESPONSE = 0x0111,
	TR111_MSGTYPE_INVALID = 0xFFFF
}tr111_message_type;

/* stun attribute type concerned */
typedef enum
{
	TR111_ATTRTYPE_MAPPEDADDRESS = 0x0001,
	TR111_ATTRTYPE_RESPONSEADDRESS = 0x0002,
	TR111_ATTRTYPE_SOURCEADDRESS = 0x0004,
	TR111_ATTRTYPE_USERNAME = 0x0006,
	TR111_ATTRTYPE_MESSAGEINTEGRITY = 0x0008,
	TR111_ATTRTYPE_ERRORCODE = 0x0009,
	/* tr111 custom */
	TR111_ATTRTYPE_CONNECTIONREQUESTBINDING = 0xC001,
	TR111_ATTRTYPE_BINDINGCHANGE = 0xC002
}tr111_attr_type;

/* work status types of control manager */
typedef enum
{
	/* not under work */
	TR111_CONTROL_STATUS_STOP = 0,
	/* waiting for task */
	TR111_CONTROL_STATUS_IDLE,
	/* dealing with task */
	TR111_CONTROL_STATUS_TASK,
	/* waiting for task again */
	TR111_CONTROL_STATUS_WAIT
}tr111_control_status;

/* stun message header */
typedef struct _msg_header
{
	unsigned short type;
	unsigned short length;
	unsigned int trans_id[4];
}msg_header;

/* stun attr header */
typedef struct _attr_header
{
	unsigned short type;
	unsigned short length;
}attr_header;

/* mapped_address attr struct */
typedef struct _mapped_address
{
	unsigned char pad;
	unsigned char family;
	unsigned short port;
	unsigned int address;
}mapped_address;

/* error code attr struct */
typedef struct _error_code
{
	unsigned short pad;
	unsigned char code_1;
	unsigned char code_2;
}error_code;

/* control manager */
typedef struct _tr111_control_mgr
{
	/* handle to tr069 manager */
	tr069_mgr *tr069_handle;
	/* status */
	tr111_control_status status;
	/* current transaction id */
	unsigned int trans_id[4];
	/* udp socket for connection request and binding request */
	int sockfd;
	/* local and public address */
	struct sockaddr_in local_addr;
	struct sockaddr_in public_addr;
	struct sockaddr_in stun_addr;
	/* send and recv buff */
	int send_len;
	int recv_len;
	unsigned char send_buff[TR111_STUN_MSG_MAX_SIZE];
	unsigned char recv_buff[TR111_STUN_MSG_MAX_SIZE];
	/* stun params */
	char stun_username[F_MAX_USERNAME_SIZE];
	char stun_password[F_MAX_PASSWORD_SIZE];
	/* keepalive period */
	unsigned int keepalive_period;
	unsigned int max_keepalive_period;
	unsigned int min_keepalive_period;
	unsigned int wait_time;
	/* some flags */
	unsigned int retry_count;
	char auth_flag;
	char binding_change_flag;
}tr111_control_mgr;

tr111_control_mgr* tr111_control_init(tr069_mgr *tr069_handle);

tr111_control_mgr* tr111_control_uninit(tr111_control_mgr *control_handle);

INT32_T tr111_control_main_task(tr111_control_mgr *control_handle);

#ifdef __cplusplus
}
#endif

#endif
