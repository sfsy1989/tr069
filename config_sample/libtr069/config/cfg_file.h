#ifndef __CFG_FILE_H__
#define __CFG_FILE_H__

#define FV_LOGO_VERSION_FILE "a.splash.version"

int cfg_get_kernelversion(char *verbuf, unsigned int bufsize);
int cfg_get_fsversion(char *verbuf, unsigned int bufsize);
int cfg_put_kernelversion(char *verbuf);
int cfg_put_fsversion(char *verbuf);
#endif

