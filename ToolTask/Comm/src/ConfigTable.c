/**
	模块说明：在Flash中创建一个简单的配置表，用来存储掉电需要保存的数据。
	存储结构：
		表1
		{
			表名：
			上一个表地址：
			下一个表地址：
		
			元素1
			{
				元素名：
				上一个元素地址：
				下一个元素地址：
				数据长度：
				数据内容：
			}
			元素2
			{
				...
			}
			...
		}
		
		表2
		{
			...
		}
		
	方法列表：
		*接口初始化：调用ConfigTableInit；
		*打开一个表：TableOpen
		*创建一个表：TableCreate
		*创建一个新的映射：TableAddItem
		*映射一个元素：TableMapItem
		*获取表的长度：TableGetItemCount
		*关闭并保存一个表：TableClose
		*关闭并保存所有的表：TableFlush

	操作逻辑：
		初始化(ConfigTableInit)-打开表(TableOpen)-映射元素(TableMapItem)-关闭表(TableFlush or TableClose)
**/
	
#include <stdio.h>
#include <string.h>
#include "ConfigTable.h"


/* Private variables ---------------------------------------------------------*/
#define MAX_NAME_LENGTH			(16)			/*String length*/
#define BLOCK_DATA_NULL			(0xffffffff)
#define BLOCK_HEAD_SIZE			(4)

#define BLOCK_TB_START			(0x05AFB6B6)
#define BLOCK_TB_END			(0x9A59B576)
#define BLOCK_ITM_START			(0x344CF6C6)
#define BLOCK_ITM_END			(0x2D2EE8C8)

/*User data area start identifier*/
const unsigned int BlockHeadIdtf[BLOCK_HEAD_SIZE] = {0XDA50B93A, 0X02F05267, 0XFBFE104A, 0XE6A110E2};

/*Core item struct, this represent the real user data*/
struct CoreItem
{
	struct CoreItem *pPrevious;			/*Pointer to previouse item*/
	struct CoreItem *pNext;				/*Pointer to next item*/
	
	cfgTableDef id;						/*This item id*/
	char name[MAX_NAME_LENGTH];			/*This item name*/	
	unsigned int mapAddr;				/*Addr in block that store real user data*/
	
	void *pData;						/*User data pointer*/
	unsigned int DataLength;			/*User data length*/
};
typedef struct CoreItem CoreItemDef;

/*Core table struct, this represent a config table*/
struct CoreTable
{	
	struct CoreTable *pPrevious;		/*Pointer to previouse table*/
	struct CoreTable *pNext;			/*Pointer to next table*/
	
	cfgTableDef id;
	char name[MAX_NAME_LENGTH];			/*Item name*/

	unsigned itemNum;					/*Total item count*/
	CoreItemDef *item;
};
typedef struct CoreTable CoreTableDef;

/*Storage item, represent a item in flash, all address is relative address*/
struct BlockItem
{
	unsigned int start;
	unsigned int preAddr;
	unsigned int nextAddr;
	unsigned int id;						/*The item id*/
	char name[MAX_NAME_LENGTH];				/*Name string*/
	unsigned int DataLength;				/*User data length*/
	unsigned char Data[];					/*User data*/
};
typedef struct BlockItem BlockItemDef;

struct BlockItemTail
{
	unsigned int end;						/*The end*/
};
typedef struct BlockItemTail BlockItemTailDef;


/*Storage table, represent a table in flash,  all address is relative address*/ 
struct BlockTable
{
	unsigned int start;
	unsigned int preAddr;
	unsigned int nextAddr;
	unsigned int id;						/*The table id*/
	char name[MAX_NAME_LENGTH];				/*Name string*/

	unsigned int firstItemAddr;				/*first item address*/
	unsigned int end;
};
typedef struct BlockTable BlockTableDef;

/*The head of block*/
struct BlockHead
{
	unsigned int identifier[BLOCK_HEAD_SIZE];				/*Represent if user data exist in config block*/
};
typedef struct BlockHead BlockHeadDef;

/*The flash manager*/
struct BlockManager
{
	unsigned int blkStartAddr;				/*Flash start address, absolute address*/
	unsigned int blockSize;					/*The block size we can use to store data*/
	unsigned int tableStartAddr;			/*Table start addr, relative address*/	
	unsigned int nextUnAllBlkAddr;			/*Next unalloced block address, new data store start here, relative address*/
};
typedef struct BlockManager BlockManagerDef;


/*Local table config*/
CfgTbConfigTypedef cfgTbConfig;

/*Initialized flag*/
unsigned char cfgInitialized = 0;

/*Avoid table open multiple times*/
unsigned char openLock = 0;

/*Table head*/
unsigned int tableCount = 0;
CoreTableDef *tableHeadList = NULL;

/*Block manager*/
BlockManagerDef blkManager;

/*Access lock*/
unsigned int accessLock = 0;

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
cfgStatus blockSaveTable(void);
cfgStatus userDataAreaCheck(void);
CoreTableDef *BlockReadTable(const char * const tableName);
cfgTableDef TableCreate(const char * const tableName);
cfgStatus TableAddItem(cfgTableDef table, const char * const itemName, void * item, unsigned int itLen);


/* Private function prototypes -----------------------------------------------*/


/*-----------------------------------------------------------*/

/*The init process, user call this to init configtable module*/
cfgStatus ConfigTableInit(CfgTbConfigTypedef *cfg)
{
	if(accessLock)return CFG_TB_BUSY;

	accessLock++;
	if(cfgInitialized != 0)
	{
		accessLock--;
		return CFG_TB_ERROR;
	}
	
	if(cfg == NULL)return CFG_TB_ERROR;	
	if((cfg->pageStartAddress <= 0x08000000) || (cfg->pageStartAddress > 0x08020000))return CFG_TB_ERROR;	
	if(cfg->pageSize % 1024)return CFG_TB_ERROR;
	if(cfg->pageNum == 0)return CFG_TB_ERROR;
	if(cfg->getSysTick == NULL)return CFG_TB_ERROR;
	if(cfg->Malloc == NULL)return CFG_TB_ERROR;
	if(cfg->Free == NULL)return CFG_TB_ERROR;
	if(cfg->GetFreeHeapSize == NULL)return CFG_TB_ERROR;
	if(cfg->FlashPageErase == NULL)return CFG_TB_ERROR;
	if(cfg->FlashUnlock == NULL)return CFG_TB_ERROR;	
	if(cfg->FlashLock == NULL)return CFG_TB_ERROR;
	if(cfg->FlashProgramWord == NULL)return CFG_TB_ERROR;

	/*Set extern interface*/
	cfgTbConfig.pageStartAddress = cfg->pageStartAddress;
	cfgTbConfig.pageSize = cfg->pageSize;
	cfgTbConfig.pageNum = cfg->pageNum;
	cfgTbConfig.getSysTick = cfg->getSysTick;
	cfgTbConfig.Malloc = cfg->Malloc;
	cfgTbConfig.Free = cfg->Free;
	cfgTbConfig.GetFreeHeapSize = cfg->GetFreeHeapSize;
	cfgTbConfig.FlashPageErase = cfg->FlashPageErase;
	cfgTbConfig.FlashUnlock = cfg->FlashUnlock;
	cfgTbConfig.FlashLock = cfg->FlashLock;
	cfgTbConfig.FlashProgramWord = cfg->FlashProgramWord;

	blkManager.blkStartAddr = cfgTbConfig.pageStartAddress;
	blkManager.blockSize = cfgTbConfig.pageSize * cfgTbConfig.pageNum;
	blkManager.tableStartAddr = sizeof(BlockHeadIdtf);
	blkManager.nextUnAllBlkAddr = 0;
	
	cfgInitialized = 1;
	
	accessLock--;
	return CFG_TB_OK;
}

/*-----------------------------------------------------------*/
/*Get uuid from a string*/
unsigned int GetUuidFromeStr(const char * const name)
{
	unsigned int uuid = 0;
	unsigned int x, seed;
	char chr;

	seed = cfgTbConfig.getSysTick();

	for(x = 0; x < MAX_NAME_LENGTH; x++)
	{
		chr = name[x];
		uuid += seed + chr << 2;

		if(chr == 0)
		{
			break;
		}
	}

	return uuid;
}

/*-----------------------------------------------------------*/
/*Copy name to des*/
void SetName(const char * const name, char * des)
{
	unsigned int x;
	
	for(x = 0; x < MAX_NAME_LENGTH; x++)
	{
		des[x] =  name[x];

		if(des[x] == 0)
		{
			break;
		}
	}
}

/*-----------------------------------------------------------*/

/*Create a user table, different table must have different tableName*/
cfgTableDef TableCreate(const char * const tableName)
{
	accessLock++;
	
	if(cfgInitialized == 0)
	{
		accessLock--;
		return CFG_TB_NULL_ERROR;
	}
	
	cfgTableDef table;;
	
	table = GetUuidFromeStr(tableName);
	
	/*Create a new table*/
	CoreTableDef *newTable = cfgTbConfig.Malloc(sizeof(CoreTableDef));
	
	/*Malloc error*/
	if(newTable == NULL)
	{
		accessLock--;
		return CFG_TB_NULL_ERROR;
	}

	newTable->id = table;
	newTable->itemNum = 0;
	SetName(tableName, newTable->name);

	/*No table in list*/
	if(tableHeadList == NULL)
	{
		tableHeadList = newTable;
		tableHeadList->pNext = NULL;
		tableHeadList->pPrevious = NULL;
	}
	/*Create a new table*/
	else
	{
		tableHeadList->pNext = newTable;
		newTable->pPrevious = tableHeadList;
	}

	tableCount += 1;

	accessLock--;
	return table;
}

/*-----------------------------------------------------------*/

/*
	功能：打开一个配置表
	说明：打开一个存储在flash中的配置表
	参数：tableName：表名字符串
	参数：flags：打开选项；
		{
			"r":以读模式打开
			"+":在配置表不存在的情况下创建一个新的配置表
		}
*/
cfgTableDef TableOpen(const char * const tableName, const char * const flags)
{	
	if(accessLock)return CFG_TB_BUSY;
	accessLock++;
	
	if(cfgInitialized == 0)
	{
		accessLock--;
		return CFG_TB_NULL_ERROR;
	}

	/*Find table*/
	CoreTableDef *pTable = BlockReadTable(tableName);
	int createNewOne = 0;

	/*Parse flags*/
	{
		char *p = (char *)flags;

		while(p && *p)
		{
			if(*p == 'r' || *p == 'R')		// Read mode
			{

			}
			else if(*p == '+')				// Create table
			{
				createNewOne = 1;
			}
			else	//Invalid parameter
			{

			}
			
			p++;
		}
	}

	/*Table does not exist, check if user want to create one*/
	if(pTable == NULL || userDataAreaCheck() != CFG_TB_OK)
	{
		if(!createNewOne)
		{
			accessLock--;
			return CFG_TB_NULL_ERROR;
		}
		else
		{
			//Create new table
			cfgTableDef tbl;
			
			tbl = TableCreate(tableName);

			accessLock--;
			return tbl;
		}
	}
	else
	{
		/*Talbe found in flash*/
		
		if(tableHeadList == NULL)
		{
			tableHeadList = pTable;
			tableHeadList->pNext = NULL;
			tableHeadList->pPrevious = NULL;
		}
		else
		{
			tableHeadList->pNext = pTable;
			pTable->pPrevious = tableHeadList;
		}

		tableCount += 1;

		accessLock--;
		return pTable->id;
	}
}

/*-----------------------------------------------------------*/

/*Close a table, data in table will be program in flash automatically*/
cfgStatus TableClose(cfgTableDef table)
{	    
	accessLock++;
	
	if(cfgInitialized == 0)
	{
		accessLock--;
		return CFG_TB_NOINIT_ERROR;
	}
	
	/*Check if table in table list*/
	CoreTableDef *cTable = NULL;
	cTable = tableHeadList;
	cfgStatus res = CFG_TB_OK;

	/*Try to find table*/
	while(cTable != NULL)
	{
		/*found user table*/
		if(cTable->id == table)
		{
			break;
		}
		
		cTable = cTable->pNext;
	}

	/*Table not found*/
	if(cTable == NULL)
	{
		accessLock--;
		return CFG_TB_NULL_ERROR;
	}
	
	tableCount -= 1;

	if(tableCount == 0)
	{
		/*Save table to block*/
		res = blockSaveTable();
	}

	accessLock--;
	return res;
}

/*Create all table and write data to block*/
cfgStatus TableFlush(void)
{	
	if(accessLock)return CFG_TB_BUSY;
	accessLock++;
	
	if(cfgInitialized == 0)
	{
		accessLock--;
		return CFG_TB_NOINIT_ERROR;
	}

	/*Check if table in table list*/
	CoreTableDef *cTable = NULL;
	cTable = tableHeadList;

	/*Try to find table*/
	while(cTable != NULL)
	{
		TableClose(cTable->id);
		
		cTable = cTable->pNext;
	}

	/*Table not found*/
	if(cTable == NULL)
	{
		accessLock--;
		return CFG_TB_NULL_ERROR;
	}

	accessLock--;
	return CFG_TB_OK;
}
/*-----------------------------------------------------------*/

/*
Add a item into table, different item must have different itemName.
Add a item to a table that create by method TableOpen will override data 
in this table.
*/
cfgStatus TableAddItem(cfgTableDef table, const char * const itemName, void * item, unsigned int itLen)
{
	if(cfgInitialized == 0)return CFG_TB_NOINIT_ERROR;
	
	CoreTableDef *cTable = NULL;
	cTable = tableHeadList;

	/*Try to find table*/
	while(cTable != NULL)
	{
		/*found user table*/
		if(cTable->id == table)
		{
			break;
		}

		cTable = cTable->pNext;
	}

	/*Table not found*/
	if(cTable == NULL)
	{
		return CFG_TB_NULL_ERROR;
	}

	/*Create new item*/
	CoreItemDef *newItem = cfgTbConfig.Malloc(sizeof(CoreItemDef));

	/*Malloc error*/
	if(newItem == NULL)
	{
		return CFG_TB_MALLOC_ERROR;
	}

	newItem->id = GetUuidFromeStr(itemName);
	SetName(itemName, newItem->name);
	newItem->pData = item;
	newItem->DataLength = itLen; 

	/*No item in table*/
	if(cTable->item == NULL)
	{
		cTable->item = newItem;
	}
	else
	{
		CoreItemDef *lastItem = cTable->item;

		/*Find last item*/		
		while(lastItem->pNext != NULL)
		{
			lastItem = lastItem->pNext;
		}

		lastItem->pNext = newItem;
		newItem->pPrevious = lastItem;
	}

	cTable->itemNum += 1;
	
	return CFG_TB_OK;
}

/*
Map a local item to table item. After open a table TableMapItem must be excute
or the original data in table will be override if excute TableAddItem.
	功能：将一个元素映射到配置表中
	说明：映射元素到配置表中，若配置表中已经包含该元素，那么在调用该方法后该元素的内容
			会被映射表中的内容所替换。若配置表中不存在该元素，用户可选择是否创建映射，
			若创建映射，配置表中会保存一个该元素的引用，并在配置表关闭的时候将该元素的
			值保存在flash中，若不创建映射，该方法不会对元素和配置表有任何更改。
	依赖：须先调用TableOpen方法
	参数：table：目标配置表
	参数：itemName：需要映射的元素名
	参数：item：需要映射的元素地址
	参数：itLen：需要映射的元素的数据长度(字节计算)
	参数：flags：映射选项；
		{
			"r":以读模式打开
			"+":在映射不存在的情况下创建一个新的映射
		}	
*/
cfgStatus TableMapItem(cfgTableDef table, const char * const itemName, 
							void * item, unsigned int itLen, const char * const flags)
{
	if(accessLock)return CFG_TB_BUSY;
	accessLock++;
	
	if(cfgInitialized == 0)
	{
		accessLock--;
		return CFG_TB_NOINIT_ERROR;
	}

	cfgStatus res = CFG_TB_NULL_ERROR;
	int createNewOne = 0;

	CoreTableDef *cTable = NULL;
	cTable = tableHeadList;

	/*Try to find table*/
	while(cTable != NULL)
	{
		/*found user table*/
		if(cTable->id == table)
		{
			break;
		}
	}

	/*Table not found*/
	if(cTable == NULL)
	{
		accessLock--;
		return CFG_TB_NULL_ERROR;
	}

	/*Parse flags*/
	{
		char *p = (char *)flags;

		while(p && *p)
		{
			if(*p == 'r' || *p == 'R')		// Read mode
			{

			}
			else if(*p == '+')				// Create table
			{
				createNewOne = 1;
			}
			else	//Invalid parameter
			{

			}
			
			p++;
		}
	}

	/*Map item*/
	CoreItemDef *cItem = cTable->item;
	while(cItem != NULL)
	{
		unsigned int x, ok = 0;
		for(x = 0; x < MAX_NAME_LENGTH; x++)
		{
			if(itemName[x] == '\0')
			{
				ok = 1;
				break;
			}

			if(itemName[x] != cItem->name[x])
			{
				ok = 0;
				break;
			}
		}

		/*Map content to item*/
		if(ok)
		{
			cItem->pData = item;
			cItem->DataLength = itLen;
			memcpy(item, (unsigned int *)cItem->mapAddr, itLen);
			res = CFG_TB_OK;
			break;
		}
		else
		{
			cItem = cItem->pNext;			
			res = CFG_TB_NULL_ERROR;
		}
	}

	//Map does not exist
	if(res == CFG_TB_NULL_ERROR && createNewOne)
	{
		res = TableAddItem(table, itemName, item, itLen);
	}

	accessLock--;
	return res;
}

/*Get item count in table*/
unsigned int TableGetItemCount(cfgTableDef table)
{
	if(accessLock)return CFG_TB_BUSY;
	accessLock++;
	
	if(cfgInitialized == 0)
	{
		accessLock--;
		return CFG_TB_NOINIT_ERROR;
	}
	
	CoreTableDef *cTable = NULL;	
	cTable = tableHeadList;

	/*Try to find table*/
	while(cTable != NULL)
	{
		/*found user table*/
		if(cTable->id == table)
		{
			break;
		}
	}

	/*Table not found*/
	if(cTable == NULL)
	{
		accessLock--;
		return 0;
	}

	accessLock--;
	return cTable->itemNum;
}

/*Put data to map block*/
cfgStatus mapBlockPutData(void * mBlock, void * data, unsigned int length)
{
	if(mBlock == NULL)
	{
		return CFG_TB_NULL_ERROR;
	}

	unsigned int addr = (unsigned int)mBlock + blkManager.nextUnAllBlkAddr;
	
	memcpy((unsigned int *)addr, data, length);
	blkManager.nextUnAllBlkAddr += length;
        
	return CFG_TB_OK;
}

/*malloc mapBlock space*/
unsigned int mapBlockMalloc(void * mBlock, unsigned int size)
{
	unsigned int startAddr;

	if(mBlock == NULL)
	{
		return BLOCK_DATA_NULL;
	}

	/*mapBlock run out*/
	if(blkManager.nextUnAllBlkAddr >= blkManager.blockSize)
	{
		return BLOCK_DATA_NULL;
	}

	/*4 bytes align*/
	if(blkManager.nextUnAllBlkAddr % 4)
	{
		blkManager.nextUnAllBlkAddr += 4 - (blkManager.nextUnAllBlkAddr % 4);
	}

	startAddr = blkManager.nextUnAllBlkAddr;
	blkManager.nextUnAllBlkAddr += size;

	return startAddr;
}

/*Get next unAlloc block address*/
unsigned int mapBlockGetNextUnAllBlkAddr(void)
{
	return blkManager.nextUnAllBlkAddr;
}

/*Read table info from blcok*/
CoreTableDef *BlockReadTable(const char * const tableName)
{	
	if(cfgInitialized == 0)return CFG_TB_NULL_ERROR;

	if(userDataAreaCheck() != CFG_TB_OK)
	{
		return CFG_TB_NULL_ERROR;
	}

	/*Find table*/
	unsigned int address = blkManager.tableStartAddr + blkManager.blkStartAddr;
	BlockTableDef *blkTb = (BlockTableDef *)(unsigned int *)address;
	CoreTableDef *cTable = NULL;

	if(blkTb->start != BLOCK_TB_START)
	{
		/*No table in block*/
		return CFG_TB_NULL_ERROR;
	}

	while(blkTb->start == BLOCK_TB_START)
	{
		unsigned int x, ok = 0;
		
		for(x = 0; x < MAX_NAME_LENGTH; x++)
		{
			/*Found table*/
			if(tableName[x] == '\0')
			{
				ok = 1;
				break;
			}
			
			if(blkTb->name[x] != tableName[x])
			{
				ok = 0;
				break;
			}
		}

		/*Try next table*/
		if(ok == 0)
		{
			if(blkTb->nextAddr == BLOCK_DATA_NULL)
			{
				break;
			}
			
			blkTb = (BlockTableDef *)(unsigned int *)blkTb->nextAddr;
		}
		else
		{			
			cTable = (CoreTableDef *)cfgTbConfig.Malloc(sizeof(CoreTableDef));
			break;
		}
	}

	if(cTable == NULL)
	{
		return CFG_TB_NULL_ERROR;
	}

	/*Read table info out*/
	cTable->id = blkTb->id;
	memcpy(cTable->name, blkTb->name, MAX_NAME_LENGTH);

	/*Read item info out*/
	unsigned int itemAddress = blkTb->firstItemAddr + blkManager.blkStartAddr;
	BlockItemDef * blkItm = (BlockItemDef*)(unsigned int *)itemAddress;

	if(blkItm->start != BLOCK_ITM_START)
	{
		/*No item in table*/
		return CFG_TB_NULL_ERROR;
	}

	CoreItemDef *itTmp = cTable->item;
	while(blkItm->start == BLOCK_ITM_START)
	{
		CoreItemDef *cItm = (CoreItemDef *)cfgTbConfig.Malloc(sizeof(CoreItemDef));
		cItm->id = blkItm->id;
		memcpy(cItm->name, blkItm->name, MAX_NAME_LENGTH);
		cItm->DataLength = blkItm->DataLength;
		cItm->mapAddr = (unsigned int)(&blkItm->Data);

		if(itTmp == NULL)
		{
			cTable->item = cItm;			
			itTmp = cTable->item;
		}
		else
		{
			itTmp->pNext = cItm;
			cItm->pPrevious = itTmp;
			itTmp = cItm;
		}
		
		/*Reach to end*/
		if(blkItm->nextAddr == BLOCK_DATA_NULL)
		{
			break;
		}
		
		blkItm = (BlockItemDef *)(unsigned int *)(blkItm->nextAddr + blkManager.blkStartAddr);
	}

	return cTable;
}
/*Program flash*/
cfgStatus flashPutData(unsigned int flashAddr,      unsigned int addr, unsigned int size)
{
	cfgStatus res = CFG_TB_OK;

	int dIndex = 0;
	int wordSize = size/4;
	unsigned int desAddress = flashAddr;
	
	if(size % 4)
	{
		wordSize = (size + 4) / 4;
	}
	
	for(dIndex = 0; dIndex < wordSize; dIndex++)
	{
		/*Check boundary*/
		if(desAddress < blkManager.blkStartAddr ||
		   desAddress >= blkManager.blkStartAddr + blkManager.blockSize)
		{
			res = CFG_TB_FLASH_ERROR;
			break;
		}
		
		if(0 != cfgTbConfig.FlashProgramWord(desAddress, ((int *)addr)[dIndex]))
		{
			res = CFG_TB_FLASH_ERROR;
			
			/*Error*/
			break;
		}

		desAddress += 4;
	}

	return res;
}

unsigned int addressAlign(unsigned int addr)
{
	unsigned int address = addr;
	
	address = address % 4 == 0 ? address : (address + 4) - address % 4;

	return address;
}

/*Save table to block*/
cfgStatus blockSaveTable(void)
{
	if(cfgInitialized == 0)return CFG_TB_NOINIT_ERROR;

	cfgStatus res = CFG_TB_OK;
	CoreTableDef *cTable = tableHeadList;
	int flashDataNeedUpdata = 0;
	
	if(cTable == NULL)
	{
		return CFG_TB_NULL_ERROR;
	}

	/*Check if flash initialized*/
	if(userDataAreaCheck() != CFG_TB_OK)
	{
		flashDataNeedUpdata = 1;
	}

	/*Find table in flash*/
	while((cTable != NULL) && (flashDataNeedUpdata == 0))
	{
		/*Find table*/
		unsigned int address = blkManager.tableStartAddr + blkManager.blkStartAddr;
		BlockTableDef *blkTb = (BlockTableDef *)(unsigned int *)address;
		int tableFound = 0;

		if(blkTb->start != BLOCK_TB_START)
		{
			/*No table in block, need program flash*/
			flashDataNeedUpdata = 1;
			break;
		}

		/*Try to find table*/
		while(blkTb->start == BLOCK_TB_START)
		{
			unsigned int x, ok = 0;
			
			for(x = 0; x < MAX_NAME_LENGTH; x++)
			{
				/*Found table*/
				if(cTable->name[x] == '\0')
				{
					ok = 1;
					break;
				}
				
				if(blkTb->name[x] != cTable->name[x])
				{
					ok = 0;
					break;
				}
			}

			/*Try next table*/
			if(ok == 0)
			{
				if(blkTb->nextAddr == BLOCK_DATA_NULL)
				{
					/*Table does not in flash, need reprogram flash*/
					flashDataNeedUpdata = 1;
					break;
				}
				
				blkTb = (BlockTableDef *)(unsigned int *)blkTb->nextAddr;
			}
			else
			{
				tableFound = 1;
				break;
			}
		}

		/*Try to find item in table*/
		if(tableFound)
		{
			/*Table found*/
			CoreItemDef *cItm = cTable->item;

			/*Find item in flash*/
			while((cItm != NULL) && (flashDataNeedUpdata == 0))
			{
				unsigned int itemAddress = blkTb->firstItemAddr + blkManager.blkStartAddr;
				BlockItemDef * blkItm = (BlockItemDef*)(unsigned int *)itemAddress;
				int itemFound = 0;
				
				if(blkItm->start != BLOCK_ITM_START)
				{
					/*No item in table*/
					return CFG_TB_NULL_ERROR;
				}

				/*Find item in falsh*/
				while(blkItm->start == BLOCK_ITM_START)
				{
					unsigned int x, ok = 0;
					
					for(x = 0; x < MAX_NAME_LENGTH; x++)
					{
						/*Found table*/
						if(cItm->name[x] == '\0')
						{
							ok = 1;
							break;
						}
						
						if(blkItm->name[x] != cItm->name[x])
						{
							ok = 0;
							break;
						}
					}

					/*Try next table*/
					if(ok == 0)
					{
						/*Reach to end*/
						if(blkItm->nextAddr == BLOCK_DATA_NULL)
						{
							/*Item does not in flash, need reprogram flash*/
							flashDataNeedUpdata = 1;
							break;
						}
						
						blkItm = (BlockItemDef *)(unsigned int *)(blkItm->nextAddr + blkManager.blkStartAddr);
					}
					else
					{
						itemFound = 1;
						break;
					}
				}

				/*Item found*/
				if(itemFound)
				{
					int index;
					
					/*Compare item contents*/
					if(cItm->DataLength != blkItm->DataLength)
					{
						/*Data length changed, need reprogram flash*/
						flashDataNeedUpdata = 1;
						break;
					}

					for(index = 0; index < cItm->DataLength; index++)
					{
						if(((unsigned char *)cItm->pData)[index] != blkItm->Data[index])
						{
							/*Data content changed, need reprogram flash*/
							flashDataNeedUpdata = 1;
							break;
						}
					}
				
				}

				/*Switch to next item*/
				cItm=cItm->pNext;
			}
		}
		
		/*Switch to next table*/
		cTable = cTable->pNext;
		
	}

	/*Program data into flash*/
	if(flashDataNeedUpdata)
	{
		cTable = tableHeadList;
		int flashRes = 1;

		/*Erase flash first*/
		cfgTbConfig.FlashUnlock();

		/*Erase pages*/
		unsigned int currentPage,
					 address,
					 nextAddress;
		
		for(currentPage = 0; currentPage < cfgTbConfig.pageNum; currentPage++)
		{
			address = blkManager.blkStartAddr + currentPage * cfgTbConfig.pageSize;
			cfgTbConfig.FlashPageErase(address);
		}
		address = blkManager.blkStartAddr + 
					sizeof(BlockHeadIdtf);

		BlockItemDef *blkItm = NULL;
		BlockTableDef *blkTb = NULL;

		/*Write table into flash*/
		while(cTable != NULL)
		{
			unsigned int preBlkTableNextTbPointAddr = BLOCK_DATA_NULL;
			unsigned int nextBlkTableAddr = BLOCK_DATA_NULL;
			unsigned int preBlkTableAddr = BLOCK_DATA_NULL;
			
			unsigned int preItemAddr = BLOCK_DATA_NULL;
			unsigned int preItemNextItemPointAddr = BLOCK_DATA_NULL;
			unsigned int nextItemAddr = BLOCK_DATA_NULL;
			unsigned int dataSize = 0;
			unsigned int mallocSize = 0;
			CoreItemDef *cItm = NULL;
			
			/*4 bytes align*/
			address = addressAlign(address);
			nextAddress = addressAlign(sizeof(BlockTableDef));

			/*Write table information*/
			mallocSize = addressAlign(sizeof(BlockTableDef));
			blkTb = cfgTbConfig.Malloc(mallocSize);
			if(blkTb == NULL)
			{
				res = CFG_TB_MALLOC_ERROR;
			}

			memset(blkTb, 0, mallocSize);
			blkTb->start = BLOCK_TB_START;
			blkTb->preAddr = preBlkTableAddr;
			blkTb->nextAddr = BLOCK_DATA_NULL;
			blkTb->id = cTable->id;
			memcpy(blkTb->name, cTable->name, MAX_NAME_LENGTH);
			blkTb->firstItemAddr = address + nextAddress - blkManager.blkStartAddr;
			blkTb->end = BLOCK_TB_END;

			flashRes = flashPutData(address, (unsigned int)blkTb, sizeof(BlockTableDef));

			if(flashRes != CFG_TB_OK)
			{
				res = CFG_TB_FLASH_ERROR;
			}
				
			if((nextBlkTableAddr != NULL) && (preBlkTableNextTbPointAddr != NULL))
			{
				flashRes = flashPutData(preBlkTableNextTbPointAddr, (unsigned int)&nextBlkTableAddr, sizeof(nextItemAddr));

				if(flashRes != CFG_TB_OK)
				{
					res = CFG_TB_FLASH_ERROR;
				}
			}
			
			cfgTbConfig.Free(blkTb);
			
			/*Write item in table into flash*/
			preBlkTableAddr = address - blkManager.blkStartAddr;
			preBlkTableNextTbPointAddr = address + (&blkTb->start - &blkTb->nextAddr) * sizeof(unsigned int);
			address += nextAddress;
			cItm = cTable->item;

			while(cItm != NULL)
			{
				unsigned int blkSize;
				dataSize = cItm->DataLength;
				address = addressAlign(address);
				blkSize = addressAlign(sizeof(BlockItemDef) + dataSize);
				nextAddress = addressAlign(blkSize);
				
				blkItm = cfgTbConfig.Malloc(blkSize);
				if(blkItm == NULL)
				{
					res = CFG_TB_MALLOC_ERROR;
				}

				memset(blkItm, 0, blkSize);
				blkItm->start = BLOCK_ITM_START;
				blkItm->preAddr = preItemAddr;
				blkItm->nextAddr = BLOCK_DATA_NULL;
				blkItm->id = cItm->id;
				memcpy(blkItm->name, cItm->name, MAX_NAME_LENGTH);
				blkItm->DataLength = cItm->DataLength;
				memcpy((void *)blkItm->Data, cItm->pData, cItm->DataLength);

				flashRes = flashPutData(address, (unsigned int)blkItm, blkSize);
				if(flashRes != CFG_TB_OK)
				{
					res = CFG_TB_FLASH_ERROR;
				}
				
				if((nextItemAddr != BLOCK_DATA_NULL) && (preItemNextItemPointAddr != BLOCK_DATA_NULL))
				{
					flashRes = flashPutData(preItemNextItemPointAddr, (unsigned int)&nextItemAddr, sizeof(nextItemAddr));

					if(flashRes != CFG_TB_OK)
					{
						res = CFG_TB_FLASH_ERROR;
					}
				}

				preItemAddr = address - blkManager.blkStartAddr;
				preItemNextItemPointAddr = address + (&blkItm->nextAddr - &blkItm->start) * sizeof(unsigned int);
				address += nextAddress;
				nextItemAddr = address - blkManager.blkStartAddr;
				
				cfgTbConfig.Free(blkItm);

				/*Switch to next item*/
				cItm = cItm->pNext;
			}


			/*Get next address*/
			nextBlkTableAddr = address - blkManager.blkStartAddr;;
			

			/*Switch to next table*/
			cTable = cTable->pNext;
		}

		/*Write head into flash at last*/
		address = blkManager.blkStartAddr;
		flashRes = flashPutData(address, (unsigned int)BlockHeadIdtf, sizeof(BlockHeadIdtf));

		if(flashRes != CFG_TB_OK)
		{
			res = CFG_TB_FLASH_ERROR;
		}

		cfgTbConfig.FlashLock();
	}


	return res;
}

/*Save table to block*/
cfgStatus blockSaveTableOld(void)
{
	if(cfgInitialized == 0)return CFG_TB_NOINIT_ERROR;

	cfgStatus res = CFG_TB_OK;

	/*Create a map for block*/
	unsigned int allocHeapSize = cfgTbConfig.pageSize * cfgTbConfig.pageNum;
	unsigned int *mBlock = cfgTbConfig.Malloc(allocHeapSize);
	
	if(mBlock == NULL)
	{
		/*Malloc error*/
		return CFG_TB_NULL_ERROR;
	}

	/*Load in ram*/
	memset(mBlock, 0xff, cfgTbConfig.pageSize * cfgTbConfig.pageNum);

	/*Write head first*/
	mapBlockPutData(mBlock, (void *)BlockHeadIdtf, sizeof(BlockHeadIdtf));

	/*Write table to mblock*/
	CoreTableDef *cTable = NULL;	
	cTable = tableHeadList;

	if(cTable == NULL)
	{
		return CFG_TB_NULL_ERROR;
	}

	unsigned int preTbAddr = BLOCK_DATA_NULL;
	unsigned int *pPreTbNext = NULL;
	
	/*Write table*/
	while(cTable != NULL)
	{
		BlockTableDef *blkTb = NULL;
		unsigned int newMallocAddr =  mapBlockMalloc(mBlock, sizeof(BlockTableDef));

		/*Malloc failed*/
		if(newMallocAddr == BLOCK_DATA_NULL)
		{
			return CFG_TB_NULL_ERROR;
		}
		
		blkTb = (BlockTableDef*)((unsigned int)mBlock + newMallocAddr);
		
		if(blkTb == NULL)
		{
			return CFG_TB_NULL_ERROR;
		}
		
		/*Set previous next pointer to this start addr*/
		if(pPreTbNext != NULL)
		{
			*pPreTbNext = newMallocAddr;
		}

		blkTb->start = BLOCK_TB_START;
		blkTb->preAddr = preTbAddr;
		blkTb->nextAddr = BLOCK_DATA_NULL;
		blkTb->id = cTable->id;
		memcpy(blkTb->name, cTable->name, MAX_NAME_LENGTH);
		blkTb->firstItemAddr = mapBlockGetNextUnAllBlkAddr();
		blkTb->end = BLOCK_TB_END;
		
		preTbAddr = newMallocAddr;
		pPreTbNext = &blkTb->nextAddr;

		/*Save items*/
		CoreItemDef *cItm = cTable->item;
		unsigned int preItmAddr = BLOCK_DATA_NULL;
		unsigned int *pPreItmNext = NULL;

		while(cItm != NULL)
		{
			BlockItemDef *blkItm = NULL;
			unsigned int newMallocAddr = mapBlockMalloc(mBlock, sizeof(BlockItemDef));

			/*Malloc failed*/
			if(newMallocAddr == BLOCK_DATA_NULL)
			{
				return CFG_TB_NULL_ERROR;
			}

			/*Store the first half*/
			blkItm = (BlockItemDef*)((unsigned int)mBlock + newMallocAddr);

			if(blkItm == NULL)
			{
				return CFG_TB_NULL_ERROR;
			}

			/*Set previous next pointer to this start addr*/
			if(pPreItmNext != NULL)
			{
				*pPreItmNext = newMallocAddr;
			}

			blkItm->start = BLOCK_ITM_START;
			blkItm->preAddr = preItmAddr;
			blkItm->nextAddr = BLOCK_DATA_NULL;
			blkItm->id = cItm->id;
			memcpy(blkItm->name, cItm->name, MAX_NAME_LENGTH);
			blkItm->DataLength = cItm->DataLength;

			preItmAddr = newMallocAddr;
			pPreItmNext = &blkItm->nextAddr;

			/*save user data*/
			newMallocAddr = mapBlockMalloc(mBlock, blkItm->DataLength);
			
			unsigned int srcAdd = (unsigned int)(cItm->pData);
			unsigned int desAddr = ((unsigned int)mBlock + newMallocAddr);
                        
			memcpy((void *) desAddr, (void * )srcAdd, blkItm->DataLength);

			/*Store last half*/
			BlockItemTailDef *blkItmTail = NULL;
			newMallocAddr = mapBlockMalloc(mBlock, sizeof(BlockItemTailDef));
			blkItmTail = (BlockItemTailDef*)((unsigned int)mBlock + newMallocAddr);
			blkItmTail->end = BLOCK_ITM_END;

			/*Switch to next table*/
			cItm=cItm->pNext;
		}
		
		cTable = cTable->pNext;
	}

	/*Compare with flash*/	
	unsigned int mBlockDataSize = mapBlockGetNextUnAllBlkAddr();
	if(0 != memcmp(mBlock, (unsigned int *)blkManager.blkStartAddr, allocHeapSize))
	{	
		/*Data changed*/
		unsigned int address,
					 currentPage,
					 dIndex = 0;
					 
		unsigned int *pmBlk = mBlock;

		/*write to flash*/
		cfgTbConfig.FlashUnlock();

		/*Erase pages*/
		for(currentPage = 0; currentPage < cfgTbConfig.pageNum; currentPage++)
		{
			address = blkManager.blkStartAddr + currentPage * cfgTbConfig.pageSize;
			cfgTbConfig.FlashPageErase(address);
		}

		address = blkManager.blkStartAddr;
		while(dIndex < mBlockDataSize)
		{
			if(0 != cfgTbConfig.FlashProgramWord(address, pmBlk[dIndex]))
			{
				res = CFG_TB_FLASH_ERROR;
				/*Error*/
				break;
			}

			dIndex++;
			address += 4;
		}
		
		cfgTbConfig.FlashLock();
	}

	/*Free tables*/
	cTable = tableHeadList;
	while(cTable != NULL)
	{		
		CoreTableDef *tbTmp = NULL;

		/*free items in table*/
		CoreItemDef *cItm = cTable->item;

		while(cItm != NULL)
		{
			CoreItemDef *itmTmp = cItm->pNext;
			cfgTbConfig.Free(cItm);
			cItm = itmTmp;
		}
		
		tbTmp = cTable->pNext;
		
		/*Free resources*/
		cTable->pNext->pPrevious = cTable->pPrevious;
		cTable->pPrevious->pNext = cTable->pNext;

		cfgTbConfig.Free(cTable);
		
		cTable = tbTmp;
	}

	/*Free heap*/
	cfgTbConfig.Free(mBlock);

	return res;
}

/*Read table from block*/
cfgStatus blockReadTable(const char * const tableName)
{
	if(cfgInitialized == 0)return CFG_TB_NOINIT_ERROR;
        
	return CFG_TB_OK;
}

/*Check if user data area initialized*/
cfgStatus userDataAreaCheck(void)
{	
	if(cfgInitialized == 0)return CFG_TB_NOINIT_ERROR;

	BlockHeadDef *pBlkHead = (BlockHeadDef*)blkManager.blkStartAddr;

	if(pBlkHead == NULL)
	{
		return CFG_TB_NULL_ERROR;
	}

	unsigned int x;
	for(x = 0; x < BLOCK_HEAD_SIZE; x++)
	{
		if(pBlkHead->identifier[x] != BlockHeadIdtf[x])
		{
			return CFG_TB_ERROR;
		}
	}

	return CFG_TB_OK;
}

