#ifndef _TPOOL_H_
#define _TPOOL_H_

typedef unsigned int tPoolHandlerDef;

typedef unsigned int tpSize32;
typedef unsigned short tpSize16;
typedef unsigned char tpSize8;

typedef enum
{
	TP_NULL_ERROR = 0,
	TP_OK,
	TP_ERROR,
}tpStatusDef;

typedef struct
{
	/*External malloc interface*/
	void *(*Malloc)(unsigned int size);
	
	/*External Free interface*/
	void (*Free)(void *pv);
	
	/*External interface to get the free heap size*/
	unsigned int (*GetFreeHeapSize)(void);

	/*The external interface to get systick tick*/
	unsigned int (*getSysTick)(void);
}tPoolConfigDef;


tpStatusDef tPoolInit(tPoolConfigDef *config);
tpStatusDef tPoolCreate(const char * const name, tpSize32 itemSize, tpSize32 length, tPoolHandlerDef *handler);
void * tPoolGetFirstRefAddr(tPoolHandlerDef pool);
void * tPoolGetOneRefAddr(tPoolHandlerDef pool);
tpStatusDef tPoolGetNextAvailableItem(tPoolHandlerDef pool, void * des, tpSize16 len);
tpStatusDef tPoolDelete(tPoolHandlerDef pool);
tpStatusDef tPoolDeleteAll(void);
tpSize32 tPoolGetOverrunCountAddr(tPoolHandlerDef pool);
tpStatusDef tPoolPutConfirm(tPoolHandlerDef pool);

#endif

