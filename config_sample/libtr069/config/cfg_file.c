#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "n_lib.h"
#include "cfg_file.h"
#include "cfg_util.h"

static int fileversion_read(char *verfilename, char *verbuf,
			    unsigned int bufsize)
{
	FILE *fd;
	char linebuf[64];
	int len;
	int ret;

	memset(linebuf,	0, 64);
	ret = __system_property_get("ro.fonsview.stb.RomVersion",linebuf);
	if ((strlen(linebuf) >= bufsize)||(ret == 0)) {
		cfg_error("version length from file is large than bufsize!\n");
		fclose(fd);
		return -1;
	}

	strcpy(verbuf, linebuf);
#if 0
	fd = fopen(verfilename, "r");
	if (NULL == fd) {
		cfg_error("open %s error!\n", verfilename);
		return -1;
	}

	if (fgets(linebuf, sizeof(linebuf), fd) == NULL) {
		cfg_error("%s gets ver error!\n", verfilename);
		return -1;
	}
	cfg_debug("%s get ver is %s\n", verfilename, linebuf);

	len = strlen(linebuf);
	if (linebuf[len - 1] == 0x0a) {
		linebuf[len - 1] = '\0';
	}

	if (strlen(linebuf) >= bufsize) {
		cfg_error("version length from file is large than bufsize!\n");
		fclose(fd);
		return -1;
	}

	strcpy(verbuf, linebuf);
	fclose(fd);
#endif
	return FV_OK;
}

static int fileversion_write(char *verfilename, const char *verbuf)
{
	FILE *fd;

	fd = fopen(verfilename, "w");
	if (NULL == fd) {
		cfg_error("open %s error!\n", verfilename);
		return -1;
	}

	fputs(verbuf, fd);
	cfg_debug("%s put ver is %s\n", verfilename, verbuf);

	fclose(fd);
	return FV_OK;
}

int cfg_get_kernelversion(char *verbuf, unsigned int bufsize)
{
	strncpy(verbuf, "2.0.0", bufsize);
	return FV_OK;
}


int cfg_get_fsversion(char *verbuf, unsigned int bufsize)
{
	return fileversion_read(FV_BASICFS_VERSION_FILE, verbuf, bufsize);
}

int cfg_put_kernelversion(char *verbuf)
{
	return FV_OK;
}

int cfg_put_fsversion(char *verbuf)
{
	fileversion_write(FV_BASICFS_VERSION_FILE, verbuf);
	return FV_OK;
}
