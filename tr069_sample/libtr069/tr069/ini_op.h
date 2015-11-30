#ifndef  __FV_CONFIG_H__
#define __FV_CONFIG_H__
#include "iniparser.h"


int FV_Config_Open( char * cfile_name);
int FV_Config_Close(void);
int FV_Config_Save(char * cfile_name);
 int  FV_Config_Get(const char * key,char * value);
int FV_Config_Set(char * cmd,char * value);

#endif
