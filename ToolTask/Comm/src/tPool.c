/*
	tPool, Used to store data.
*/
#include <stdio.h>
#include <string.h>
#include "tPool.h"

/*Max name string length*/
#define MAX_NAME_LENGTH 			(16)
#define DATA_AVAILABLE				(0xA8A8A8A8)
#define DATA_INITIALIZED			(0xFFFFFFFF)

struct tPoolNode
{
	struct tPoolNode * previous;	/*Previous pool reference*/
	struct tPoolNode * next;		/*Next pool reference*/

	tpSize32 overrunCount;			/*The overrun times*/
	tPoolHandlerDef handler;		/*The handler of pool*/
	tpSize32 fishSize;				/*The datatype size in pool*/
	tpSize32 fishCapacity;			/*The max data count that pool can contain*/
	char name[MAX_NAME_LENGTH];		/*Pool name*/

	void *poolStart;				/*The lower limit of the pool*/
	void *pool;						/*Reference to the pool data*/	
	void *Pointer;					/*current item pointer in pool*/
	void *Available;				/*The item that has available data*/
	tpSize32 poolFence;				/*The upper limit of the pool*/
};
typedef struct tPoolNode tPoolNodeDef;

tPoolNodeDef *tPoolFirstNode = NULL;


tpSize8 tpInterfaceInitialized = 0;
tPoolConfigDef tpConfig;

/*-----------------------------------------------------------*/
/*Get uuid from a string*/
unsigned tpGetUuidFromeStr(const char * const name)
{
	unsigned int uuid = 0;
	unsigned int x, seed;
	char chr;

	seed = tpConfig.getSysTick();

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
void tpSetName(const char * const name, char * des)
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


/*Init tPool interface*/
tpStatusDef tPoolInit(tPoolConfigDef *config)
{
	if(config == NULL)return TP_ERROR;
	if(config->Malloc == NULL)return TP_ERROR;
	if(config->Free == NULL)return TP_ERROR;
	if(config->GetFreeHeapSize == NULL)return TP_ERROR;
	if(config->getSysTick == NULL)return TP_ERROR;

	tpConfig.Malloc = config->Malloc;
	tpConfig.Free = config->Free;
	tpConfig.GetFreeHeapSize = config->GetFreeHeapSize;
	tpConfig.getSysTick = config->getSysTick;

	tpInterfaceInitialized = 1;

	return TP_OK;
}

/*Find pool by name*/
tPoolNodeDef * findPoolByName(const char * const name)
{
	/*Check if pool exist*/
	if(tPoolFirstNode == NULL)
	{
		return NULL;
	}
	
	tPoolNodeDef *node = tPoolFirstNode;

	while(node != NULL)
	{
		tpSize8 match = 1;
		tpSize8 x;
		
		for(x = 0; x < MAX_NAME_LENGTH; x++)
		{
			if(name[x] != node->name[x])
			{
				match = 0;
				break;
			}

			if(name[x] == 0)
			{
				match = 1;
				break;
			}
		}

		/*Found pool with this name*/
		if(match == 1)
		{
			return NULL;
		}
		
		node = node->next;
	}

	return node;
}

/*Find pool by handler*/
tPoolNodeDef * findPoolByHandler(tPoolHandlerDef pool)
{
	/*Check if pool exist*/
	if(tPoolFirstNode == NULL)
	{
		return NULL;
	}

	tPoolNodeDef *node = tPoolFirstNode;

	while(node != NULL)
	{
		if(node->handler == pool)
		{
			return node;
		}

		node = node->next;
	}

	return NULL;
}

/*Create a new pool*/
tpStatusDef tPoolCreate(const char * const name, tpSize32 itemSize, tpSize32 length, tPoolHandlerDef *handler)
{
	if(tpInterfaceInitialized == 0)return TP_NULL_ERROR;

	/*Check repeat*/
	tPoolNodeDef * node = NULL;
	// Make sure every item's first addr is 4 bytes alignment
	tpSize16 pItemSize = itemSize % sizeof(tpSize32) ? \
						itemSize + sizeof(tpSize32) - itemSize % sizeof(tpSize32) : \
						itemSize;
	tpSize16 typeSize = pItemSize + 2 * sizeof(tpSize32);	//Each data item has a word to store identifier, and one word for alignment
	tpSize32 nodeAddr = 0;

	if((node = findPoolByName(name)) != NULL)
	{
		return TP_ERROR;
	}

	/*Create a node first*/
	tPoolNodeDef *nNode = (tPoolNodeDef *)tpConfig.Malloc(sizeof(tPoolNodeDef));	

	if(nNode == NULL)
	{
		return TP_ERROR;
	}
	
	nodeAddr = (tpSize32)nNode;
	nNode->previous = NULL;
	nNode->next = NULL;
	nNode->handler = tpGetUuidFromeStr(name);
	tpSetName(name, nNode->name);
	nNode->fishSize = typeSize;
	nNode->fishCapacity = length;
	nNode->overrunCount = 0;
	
	/*Attach data to this node*/
	tpSize32 mcSize = typeSize * length;
	void * data = tpConfig.Malloc(mcSize);
	
	if(data == NULL)
	{
		/*Malloc failed*/
		tpConfig.Free(nNode);

		return TP_ERROR;
	}
	
	memset(data, 0xff, mcSize);
	nNode->poolStart = data;
	// First 1 word is the identifier
	nNode->pool = (void *)((tpSize32)nNode->poolStart + sizeof(tpSize32));
	// Make sure the first pool's address is 4 bytes alignment
	nNode->pool = (tpSize32)nNode->pool % sizeof(tpSize32) ? \
					(void *)(((tpSize32)nNode->pool + sizeof(tpSize32)) - (tpSize32)nNode->pool % sizeof(tpSize32)) : \
					nNode->pool;
	nNode->Pointer = nNode->pool;
	nNode->Available = NULL;
	nNode->poolFence = (tpSize32)nNode->pool + (nNode->fishSize * nNode->fishCapacity);	
	*handler = (tPoolHandlerDef)nodeAddr;
	
	/*This is the first node*/
	if(tPoolFirstNode == NULL)
	{
		tPoolFirstNode = nNode;

		return TP_OK;
	}

	/*Attach node to the end*/
	node = tPoolFirstNode;
	while(node->next != NULL)
	{
		node = node->next;
	}

	node->next = nNode;
	nNode->previous = node;

	return TP_OK;
}
/*Remove a pool from ram*/
tpStatusDef tPoolDelete(tPoolHandlerDef pool)
{
	if(tpInterfaceInitialized == 0)return TP_NULL_ERROR;

	/*Find pool*/
	tPoolNodeDef * node = NULL;
	if((node = findPoolByHandler(pool)) != NULL)
	{		
		return TP_ERROR;
	}

	/*Detach node from list*/
	if(node->previous != NULL)
	{
		node->previous->next = node->next;
	}

	if(node->next != NULL)
	{
		node->next->previous = node->previous;
	}

	/*Free node resources*/
	if(node->pool != NULL)
	{
		tpConfig.Free(node->poolStart);
	}
	tpConfig.Free(node);

	return TP_OK;
}

/*Delete all node in list*/
tpStatusDef tPoolDeleteAll(void)
{
	if(tpInterfaceInitialized == 0)return TP_NULL_ERROR;

	tPoolNodeDef * node = tPoolFirstNode;
	while(node != NULL)
	{
		tPoolNodeDef * tmNode = node;
		node = node->next;
		
		if(tmNode->pool != NULL)
		{
			tpConfig.Free(tmNode->poolStart);
			tpConfig.Free(tmNode);
		}
	}

	return TP_OK;
}
/*Get first item's address*/
void * tPoolGetFirstRefAddr(tPoolHandlerDef pool)
{
	if(tpInterfaceInitialized == 0)return NULL;

	if(pool == 0)return NULL;

	tPoolNodeDef *tPool = (tPoolNodeDef *)pool;

	return tPool->pool;
}
/*Get a new item's address*/
void * tPoolGetOneRefAddr(tPoolHandlerDef pool)
{
	if(tpInterfaceInitialized == 0)return NULL;

	if(pool == 0)return NULL;

	tPoolNodeDef *tPool = (tPoolNodeDef *)pool;
	
	/*Get current pointer*/
	tpSize32 addrNext = (tpSize32)tPool->Pointer + tPool->fishSize;
	void * nextPointer = NULL;
	
	if (tPool->Available == NULL)
	{
		tPool->Available = tPool->Pointer;
	}

	/*Pointer point to next*/
	if(addrNext >= tPool->poolFence)
	{
		nextPointer = tPool->pool;
	}
	else
	{
		nextPointer = (void*)addrNext;
	}

	if((*(tpSize32 *)((tpSize32)nextPointer - sizeof(tpSize32)) == DATA_INITIALIZED))
	{
		/*make the last item avaiable*/
		*(tpSize32 *)((tpSize32)tPool->Pointer - sizeof(tpSize32)) = DATA_AVAILABLE;
		
		tPool->Pointer = nextPointer;
	}
	else
	{
		/*make the last item not avaiable*/
		*(tpSize32 *)((tpSize32)tPool->Pointer - sizeof(tpSize32)) = DATA_INITIALIZED;
		
		tPool->overrunCount += 1;
	}

	return tPool->Pointer;
}
/*Get an avaiable item's address*/
tpStatusDef tPoolGetNextAvailableItem(tPoolHandlerDef pool, void * des, tpSize16 len)
{
	if(tpInterfaceInitialized == 0)return TP_NULL_ERROR;

	if(pool == 0)return TP_NULL_ERROR;

	tPoolNodeDef *tPool = (tPoolNodeDef *)pool;

	if ( tPool->Available == NULL)
	{
		return TP_ERROR;
	}

	if ((*(tpSize32 *)((tpSize32)tPool->Available - sizeof(tpSize32)) == DATA_AVAILABLE))
	{
		memcpy(des, tPool->Available, len);
		*(tpSize32 *)((tpSize32)tPool->Available - sizeof(tpSize32)) = DATA_INITIALIZED;

		/*Got to next*/
		tpSize32 nextAddr = (tpSize32)tPool->Available + tPool->fishSize;
		if(nextAddr >= tPool->poolFence)
		{
			tPool->Available = tPool->pool;
		}
		else
		{
			tPool->Available = (void *)(nextAddr);
		}
		
		return TP_OK;
	}

	return TP_ERROR;
}
/*Make current item available*/
tpStatusDef tPoolPutConfirm(tPoolHandlerDef pool)
{
	if(tpInterfaceInitialized == 0)return TP_NULL_ERROR;

	if(pool == 0)return TP_NULL_ERROR;

	tPoolNodeDef *tPool = (tPoolNodeDef *)pool;

	if (tPool->Pointer == NULL)
	{
		return TP_NULL_ERROR;
	}

	tpSize32 preAddr = (tpSize32)tPool->Pointer;
	
	if(preAddr < (tpSize32)tPool->pool)
	{
		preAddr = (tpSize32)tPool->pool;
	}

	{
		*(tpSize32 *)(preAddr - sizeof(tpSize32)) = DATA_AVAILABLE;
		return TP_OK;
	}
}
/*Get the overrun count*/
tpSize32 tPoolGetOverrunCountAddr(tPoolHandlerDef pool)
{
	if(tpInterfaceInitialized == 0)return NULL;

	if(pool == 0)return NULL;

	tPoolNodeDef *tPool = (tPoolNodeDef *)pool;

	return tPool->overrunCount;
}
