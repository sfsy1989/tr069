/********************************************************************
	Created By Ash, 2012/07/10
	Model: Tr069
	Sub Model: CU USB Config
	Function: China Unicom's USB Config
********************************************************************/
#include <n_lib.h>
#include <n_stb_tr069.h>
#include <md5-util.h>

#include "cu_usb_config.h"
#include "tr069_parameter.h"
#include "tr069_interface.h"
#include "tr069.h"

/* extern */
extern tr069_mgr *g_tr069;
extern INT8_T* tr069_parameter_name_table[TR069_PARAM_NUM];
/*
int read_hex_string(char *string_buff, char* start_tag, 
	char *end_tag, unsigned char *recv_buff)
{
	char *pname;
	char *pvalue;
	char *pcl;
	char *pread_byte;
	int value_len;
	int index;
	char tmphex[3];
	
	pname = strstr(string_buff, start_tag);
	pvalue = pname + 2;
	pcl = strstr(pvalue, end_tag);
	value_len = pcl - pvalue;
	if (value_len % 2 == 0) {
		memcpy(tmphex, pvalue, 2);
		tmphex[2] = 0;
		sscanf(tmphex, "%x", recv_buff);
		pread_byte = pvalue + 2;
	}
	else {
		tmphex[0] = '0';
		tmphex[1] = *pvalue;
		tmphex[2] = 0;
		sscanf(tmphex, "%x", recv_buff);
		pread_byte = pvalue + 1;
	}
	index = 1;
	while (pread_byte < pcl) {
		memcpy(tmphex, pread_byte, 2);
		tmphex[2] = 0;
		sscanf(tmphex, "%x", recv_buff + index);
		pread_byte += 2;
		index += 1;
	}
	
	return index;
}*/

static const int base64_dec_map[128] =
{
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127,  62, 127, 127, 127,  63,  52,  53,
     54,  55,  56,  57,  58,  59,  60,  61, 127, 127,
    127,  64, 127, 127, 127,   0,   1,   2,   3,   4,
      5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
     25, 127, 127, 127, 127, 127, 127,  26,  27,  28,
     29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
     39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
     49,  50,  51, 127, 127, 127, 127, 127
};

int base64_decode( unsigned char *dst, int *dlen,
                   unsigned char *src, int  slen )
{
	int i, j, n;
	unsigned long x;
	unsigned char *p;

	for( i = j = n = 0; i < slen; i++ ) {
		if( ( slen - i ) >= 2 &&
		    src[i] == '\r' && src[i + 1] == '\n' )
			continue;

		if( src[i] == '\n' )
			continue;

		if( src[i] == '=' && ++j > 2 )
			return( ERR_BASE64_INVALID_CHARACTER );

		if( src[i] > 127 || base64_dec_map[src[i]] == 127 )
			return( ERR_BASE64_INVALID_CHARACTER );

		if( base64_dec_map[src[i]] < 64 && j != 0 )
			return( ERR_BASE64_INVALID_CHARACTER );

		n++;
	}

	if( n == 0 )
		return( 0 );

	n = ( ( n * 6 ) + 7 ) >> 3;

	if( *dlen < n ) {
		*dlen = n;
		return( ERR_BASE64_BUFFER_TOO_SMALL );
	}

	for( j = 3, n = x = 0, p = dst; i > 0; i--, src++ ) {
		if( *src == '\r' || *src == '\n' )
			continue;

		j -= ( base64_dec_map[*src] == 64 );
		x  = ( x << 6 ) | ( base64_dec_map[*src] & 0x3F );

		if( ++n == 4 ) {
			n = 0;
			*p++ = (unsigned char) ( x >> 16 );
			if( j > 1 ) *p++ = (unsigned char) ( x >> 8 );
			if( j > 2 ) *p++ = (unsigned char )  x;
		}
	}

	*dlen = p - dst;

	return( 0 );
}


int base64_to_bin_file(char *base64_file_path, char *bin_file_path)
{
	FILE *base64_file = NULL;
	FILE *bin_file = NULL;
	char *base64_buff = NULL;
	char *bin_buff = NULL;
	unsigned int base64_size=0;
	unsigned int bin_size = 0;
	
	if (base64_file_path == NULL || bin_file_path == NULL) {
		tr069_error("base64 to bin param error\n");
		return -1;
	}
	
	base64_file = fopen(base64_file_path, "r");
	if (base64_file == NULL) {
		tr069_error("base64 file open error\n");
		goto error_exit;
	}
	fseek(base64_file, 0, SEEK_END);
	base64_size = ftell(base64_file);
	fseek(base64_file, 0, SEEK_SET);
	base64_buff = malloc(base64_size + 1);
	if (base64_buff == NULL) {
		tr069_error("base64 buff malloc error\n");
		goto error_exit;
	}
	memset(base64_buff, 0, base64_size + 1);
	
	if (fread(base64_buff, 1, base64_size, base64_file) < base64_size) {
		tr069_error("base64 file read error\n");
		goto error_exit;
	}
	
	bin_size = base64_size * 2;
	bin_buff = malloc(bin_size);
	if (base64_buff == NULL) {
		tr069_error("bin buff malloc error\n");
		goto error_exit;
	}
	memset(bin_buff, 0, bin_size);
	
	base64_decode((unsigned char*)bin_buff, (int*)&bin_size, 
		(unsigned char*)base64_buff, base64_size);
	tr069_debug("bin buff size is %d\n", bin_size);
	
	bin_file = fopen(bin_file_path, "w");
	if (bin_file == NULL) {
		tr069_error("bin file open error\n");
		goto error_exit;
	}
	fwrite(bin_buff, 1, bin_size, bin_file);
	
	fclose(base64_file);
	fclose(bin_file);
	free(base64_buff);
	free(bin_buff);
	tr069_notice("success in convert key file\n");
	
	return 0;	
	
error_exit:
	
	if (base64_file != NULL)
		fclose(base64_file);
	if (bin_file != NULL)
		fclose(bin_file);
	if (base64_buff != NULL)
		free(base64_buff);
	if (bin_buff != NULL)
		free(bin_buff);
	
	return -1;
}

RSA* read_rsa_key_file(char *key_file_path)
{
	RSA *key = NULL;
	FILE *key_file = NULL;
	unsigned int key_size = 0;
	unsigned char *key_buff = NULL;
	
	if (key_file_path == NULL) {
		tr069_error("read rsa key file param error\n");
		return NULL;
	}
	
	tr069_debug( "key file = %s\n", key_file_path);
	key_file = fopen(key_file_path, "rb");
	if (key_file == NULL) {
		tr069_error("key file open error\n");
		return NULL;
	}
	
	fseek(key_file, 0, SEEK_END);
	key_size = ftell(key_file);
	tr069_debug( "key_size = %d\n", key_size);
	fseek(key_file, 0, SEEK_SET);
	
	key_buff = malloc(key_size + 1);
	fread(key_buff, 1, key_size, key_file);
	fclose(key_file);
	unlink(key_file_path);
	
	key = d2i_RSA_PUBKEY(NULL, (const unsigned char**)&key_buff, key_size);
	if (key != NULL) {
		tr069_notice("success get rsa public key\n");
	}
	else {
		tr069_error("failed to get rsa public key\n");
	}

	/* dont free(key_buff) or it will crash */
	return key;
}

void print_hex_data(unsigned char *hex_data, int data_len)
{
	int i;
	for (i = 0; i < data_len; i++) {
		tr069_debug("%02X", *(hex_data + i));
	}
	tr069_debug("\n");
	return;
}

int cu_public_decrypt(char *enc_file_path, char *dec_file_path, RSA *key)
{
	FILE *enc_file = NULL;
	FILE *dec_file = NULL;
	unsigned char *enc_buff = NULL;
	unsigned char *dec_buff = NULL;
	unsigned char *tmp_buff = NULL;
	unsigned int block_num;
	int enc_size = 0;
	int dec_size = 0;
	int i;
	int ret;
	unsigned char *ptr_to_free[3];

	if ((enc_file_path == NULL) || (dec_file_path == NULL))	{
		tr069_error("cu public decrypt param error\n");
		return -1;
	}
	
	enc_file = fopen(enc_file_path, "rb");
	if (enc_file == NULL) {
		tr069_error("enc file open error\n");
		return -1;
	}
	
	fseek(enc_file, 0, SEEK_END);
	enc_size = ftell(enc_file);
	fseek(enc_file, 0, SEEK_SET);
	
	enc_buff = malloc(enc_size + 1);
	fread(enc_buff, 1, enc_size, enc_file);
	
	tr069_debug("cryto file data:\n");
	print_hex_data(enc_buff, enc_size);
	fclose(enc_file);
	
	dec_buff = malloc(enc_size * 2);
	tmp_buff = malloc(enc_size * 2);
	block_num = enc_size / RSA_BLOCK_SIZE;
	ptr_to_free[0] = enc_buff;
	ptr_to_free[1] = dec_buff;
	ptr_to_free[2] = tmp_buff;
	
	for (i = 0; i < block_num; ++i) {
		ret = RSA_public_decrypt(RSA_BLOCK_SIZE, (const unsigned char *)enc_buff,
			(unsigned char *)dec_buff, key, RSA_NO_PADDING);
		tr069_debug("RSA_public_decrypt return = %d\n", ret);
		if (ret <= 0) {
			tr069_error("rsa public decrypt block %d error\n", i);
			ret = -1;
			goto exit_proc;
		}
		//g_hexdump(dec_buff,256,"decrypt content:",__func__,__LINE__);
		//very strange encrypt way. 
		//1. first 128 bytes are 00, second 128 of 256 bytes are text content.
		//2. In last block, the clear content of last dec_size%128 length is in back end.
		//copy decrypted block.
		if (i < block_num - 1) {
			memcpy(tmp_buff, dec_buff + 128, 128);
			tmp_buff += 128;
			dec_size += 128;
		}
		else {
			//In last block, the clear content of last dec_size%128 length is in back end.
			//found first NOT 00 bianry.
			int j = 0;
			//skip 00 byte.
			while (*(dec_buff + 128 + j) == 0 && j < 128) {
				j++;
			}
			memcpy(tmp_buff, dec_buff + 128 + j, 128 - j);
			tmp_buff += (128 - j);
			dec_size += (128 - j);
		}
		enc_buff += RSA_BLOCK_SIZE;
	}
	
	dec_file = fopen(dec_file_path, "w");
	if (dec_file == NULL) {
		tr069_error("dec file open error\n");
		ret = -1;
		goto exit_proc;
	}
	tr069_notice("dec data size is %d\n", dec_size);
	fwrite(ptr_to_free[2], 1, dec_size, dec_file);
	print_hex_data(ptr_to_free[2], dec_size);
	fclose(dec_file);
	ret = 0;

exit_proc:
	
	free(ptr_to_free[0]);
	free(ptr_to_free[1]);
	free(ptr_to_free[2]);
	
	return ret;
}

/* return: -1 config file open error
           -2 config file parse error
           0 config ok
*/
int cu_usb_config()
{
	char usb_cfg_filename[256] = {0};
	unsigned char cname[TR069_MAX_PARAM_NAME_LEN] = {0};
	unsigned char cvalue[TR069_MAX_PARAM_VALUE_LEN] = {0};
	unsigned char value_buff[TR069_MAX_PARAM_VALUE_LEN] = {0};
	unsigned char file_buff[10240] = {0};
	FILE *cfg_file = NULL;
	MD5Context md5_ctx;
	int ret = -1, i;
	unsigned char *pbegin = NULL;
	unsigned char *pend = NULL;
	unsigned char *pname = NULL;
	unsigned char *pvalue = NULL;
	unsigned char *peq = NULL;
	unsigned char *pcl = NULL;
	unsigned char md5_digest[16] = {0};
	unsigned char mfile[33] = {0};
	unsigned char line_num = 0;
	RSA *key = NULL;

	/* try 2 usb */
	tr069_get_config(TR069_PARAM_STBID, (char*)value_buff, sizeof(value_buff));
	snprintf(usb_cfg_filename, sizeof(usb_cfg_filename), "%s%s%s",
		CU_USB_CFG_PATH_PREFIX0, value_buff, 
		CU_USB_CFG_PATH_SUFFIX);
	tr069_notice("try config file:%s\n", usb_cfg_filename);

	cfg_file = fopen(usb_cfg_filename, "r");
	if (NULL == cfg_file) {
		tr069_error("open %s failed, ", usb_cfg_filename);
		snprintf(usb_cfg_filename, sizeof(usb_cfg_filename), "%s%s%s",
			CU_USB_CFG_PATH_PREFIX1, value_buff,
			CU_USB_CFG_PATH_SUFFIX);
		tr069_notice("try config file:%s\n", usb_cfg_filename);

		cfg_file = fopen(usb_cfg_filename, "r");
		if (NULL == cfg_file) {
			tr069_error("open %s failed, there is no config file\n", usb_cfg_filename);
			return -1;
		}
	}

	/* just verify the path and start decrypt */
	fclose(cfg_file);
	
	/* convert base64 to bin */
	if (base64_to_bin_file(BASE64_KEY_FILE_PATH, BIN_KEY_FILE_PATH) == -1) {
		tr069_error("base 64 to bin key file error\n");
		return -2;
	}
	
	/* read bin key file */
	key = read_rsa_key_file(BIN_KEY_FILE_PATH);
	if (key == NULL) {
		tr069_error("read key file error\n");
		return -2;
	}

	/* rsa decrypt */
	ret = cu_public_decrypt(usb_cfg_filename, CONFIG_DEC_FILE_PATH, key);
	RSA_free(key);
	if (ret != 0) {
		tr069_error("cu public decrypt error\n");
		return -2;
	}

	/* read config file content */
	cfg_file = fopen(CONFIG_DEC_FILE_PATH, "r");
	ret = fread(file_buff, 1, sizeof(file_buff), cfg_file);
	fclose(cfg_file);
	unlink(CONFIG_DEC_FILE_PATH);
	if (ret <= CU_USB_CFG_MIN_LEN) {
		tr069_error("read config file length error\n");
		return -2;
	}

	/* check for begin and end tag */
	pbegin = file_buff + 32;
	if (memcmp(pbegin, CU_USB_CFG_BEGINTAG, strlen(CU_USB_CFG_BEGINTAG)) != 0) {
		tr069_error("begin tag error\n");
		return -2;
	}
	pend = file_buff + ret - strlen(CU_USB_CFG_ENDTAG);
	if (memcmp(pend, CU_USB_CFG_ENDTAG, strlen(CU_USB_CFG_ENDTAG)) != 0) {
		tr069_error("end tag error\n");
		return -2;
	}

	/* calculate mfile` */
	MD5Init(&md5_ctx);
	MD5Update(&md5_ctx, pbegin, ret - 32);
	MD5Final(md5_digest, &md5_ctx);
	for(i = 0; i < 16; i++) {
		sprintf((char*)mfile + 2 * i, "%02x", md5_digest[i]);
	}
	tr069_notice("mfile` is %s\n", mfile);

	/* check mfile` */
	if (memcmp(mfile, file_buff, 32) != 0) {
		tr069_error("md5 check error\n");
		return -2;
	}

	/* now the config file is safe, go on check stbid */
	pname = pbegin + strlen(CU_USB_CFG_BEGINTAG);
	if (memcmp(pname, tr069_parameter_name_table[TR069_PARAM_STBID], 
		strlen(tr069_parameter_name_table[TR069_PARAM_STBID])) != 0) {
		tr069_error("%s name error\n", tr069_parameter_name_table[TR069_PARAM_STBID]);
		return -2;
	}
	peq = (unsigned char*)strstr((char*)pname, "=");
	pcl = (unsigned char*)strstr((char*)pname, "\n");
	if (peq == NULL || pcl == NULL || peq >= pend || pcl >= pend || peq >= pcl) {
		tr069_error("%s format error\n", tr069_parameter_name_table[TR069_PARAM_STBID]);
		return -2;
	}
	pvalue = peq + 1;
	if (strlen((char*)value_buff) != (pcl - pvalue) || memcmp(value_buff, pvalue, pcl - pvalue) != 0) {
		tr069_error("%s value error\n", tr069_parameter_name_table[TR069_PARAM_STBID]);
		return -2;
	}
	pname = pcl + 1;
	line_num++;

	/* stbid ok, read and set config */

	/* if we want to prompt user about config confirm, add it here */
	
	while (pname < pend) {
		peq = (unsigned char*)strstr((char*)pname, "=");
		pcl = (unsigned char*)strstr((char*)pname, "\n");
		
		if (peq == NULL || pcl == NULL || peq >= pend || pcl >= pend || peq >= pcl) {
			tr069_error("config name in line %d error\n", line_num);
			ret = -2;
			break;
		}
		
		memcpy(cname, pname, peq - pname);
		cname[peq - pname] = 0;
		
		pvalue = peq + 1;
		if (pvalue == pcl) {
			/* empty parameter */
			cvalue[0] = 0;
		}
		else {
			memcpy(cvalue, pvalue, pcl - pvalue);
			cvalue[pcl - pvalue] = 0;
		}

		tr069_debug("set parameter %s = %s\n", cname, cvalue);
		tr069_parameter_set_value(g_tr069->parameter_handle, (char*)cname, (char*)cvalue, 1);
		
		/* next config */
		pname = pcl + 1;
		line_num++;
	}

	/* save config so it can stay if no error */
	if (ret == -2) {
		tr069_error("read and set config file error, dont save\n");
		return -2;
	}
	else {
		tr069_save_all_config();
	}

	return 0;
}

