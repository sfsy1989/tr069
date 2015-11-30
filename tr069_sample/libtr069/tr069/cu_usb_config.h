/********************************************************************
	Created By Ash, 2012/07/10
	Model: Tr069
	Sub Model: CU USB Config
	Function: China Unicom's USB Config
********************************************************************/
#ifndef _TR069_CU_USB_CONFIG_
#define _TR069_CU_USB_CONFIG_
	
#include <n_stb_tr069.h>
#include <stdio.h>
#include <string.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "tr069_common.h"

#ifdef __cplusplus
	extern "C" {
#endif

#define CU_USB_CFG_PATH_PREFIX0 "/usb/disk0/CUSTBConfig/"
#define CU_USB_CFG_PATH_PREFIX1 "/usb/disk1/CUSTBConfig/"
#define CU_USB_CFG_PATH_SUFFIX "_UNICOM_CONFIG.STB"
#define CU_USB_CFG_BEGINTAG "[begin]\n"
#define CU_USB_CFG_ENDTAG "[end]"
#define CU_USB_CFG_MIN_LEN 45

#define BASE64_KEY_FILE_PATH "/ipstb/etc/pubkey_base64.dat"
#define BIN_KEY_FILE_PATH "/tmp/pubkey_bin.dat"
#define CONFIG_DEC_FILE_PATH "/tmp/dec_file"

#define RSA_BLOCK_SIZE 256

#define ERR_BASE64_INVALID_CHARACTER -1
#define ERR_BASE64_BUFFER_TOO_SMALL -2

int cu_usb_config();

#ifdef __cplusplus
}
#endif

#endif

