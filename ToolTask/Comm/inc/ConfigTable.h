#ifndef _CONFIGTABLE_H_
#define _CONFIGTABLE_H_

#include <stdio.h>

typedef unsigned int cfgTableDef;
typedef unsigned int cfgItemDef;

typedef enum
{
	CFG_TB_NULL_ERROR = 0,
	CFG_TB_OK,
	CFG_TB_ERROR,
	CFG_TB_MALLOC_ERROR,
	CFG_TB_NOINIT_ERROR,
	CFG_TB_FLASH_ERROR,
	CFG_TB_BUSY,
}cfgStatus;

typedef struct
{
	/*The user data page start address*/
	unsigned int pageStartAddress;

	/*One page size in flash*/
	unsigned int pageSize;

	/*How many page we can use*/
	unsigned int pageNum;

	/*The external interface to get systick tick*/
	unsigned int (*getSysTick)(void);
	
	/*External malloc interface*/
	void *(*Malloc)(unsigned int size);
	
	/*External Free interface*/
	void (*Free)(void *pv);

	/*External interface to get the free heap size*/
	unsigned int (*GetFreeHeapSize)(void);

	/*External interface to erase flash page*/
	void (*FlashPageErase)(unsigned int pageAddress);

	/*External interface to unlock flash*/
	unsigned int (*FlashUnlock)(void); 

	/*External interface to lock flash*/
	unsigned int (*FlashLock)(void); 

	/*External interface to program a word to flash*/
	unsigned int (*FlashProgramWord)(unsigned int address, unsigned int data); 
}CfgTbConfigTypedef;

/*User interface*/
extern cfgStatus ConfigTableInit(CfgTbConfigTypedef *cfg);
extern cfgTableDef TableOpen(const char * const tableName, const char * const flags);
extern cfgStatus TableMapItem(cfgTableDef table, const char * const itemName, void * item, unsigned int itLen, const char * const flags);
extern cfgStatus TableFlush(void);
extern unsigned int TableGetItemCount(cfgTableDef table);

#endif

