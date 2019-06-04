#ifndef _SLAVETOOL_H_
#define _SLAVETOOL_H_

#include "interface.h"


void McuInit(void *);
void McuDeInit(void * );
void McuBasicTaskProc(void *);
void HostCmdProc(unsigned char *, unsigned short, void *);
int HostRxDecode(unsigned char *pData, unsigned short length, void *pBaseInfo);




#endif	/*_SLAVE_TOOL_H_*/
