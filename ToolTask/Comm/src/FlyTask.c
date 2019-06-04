/* USER CODE BEGIN Includes */
#include <string.h>
#include "main.h"

#include "FlyTask.h"
#include "cmsis_os.h"
#include "interface.h"
#include "FlyConfig.h"

#include "slaveTool.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
#define configQUEUE_LENGTH_RX_FROM_Host			(256)
#define configQUEUE_SIZE_ITEM_RX_FROM_Host		(sizeof(uint8))

#define configQUEUE_LENGTH_TX_TO_Host			(30)
#define configQUEUE_SIZE_ITEM_TX_TO_Host		(sizeof(TxToHostFrameTypedef*))

/*任务名*/
#define	Basic_TASK_NAME						"BasicTask"
#define	ComRx_TASK_NAME						"ComRxTask"
#define	ComTx_TASK_NAME						"ComTxTask"
/*任务名*/

/*Task handle*/
TaskHandle_t	tskHandleBasic;
TaskHandle_t	tskHandleComRx;
TaskHandle_t	tskHandleComTx;
/*Task handle*/

/* Add queues, ... */
static QueueHandle_t xQueueRxFromHost	 		= NULL;
static QueueHandle_t xQueueTxToHost	 			= NULL;

/* System reset reason */
static uint8 sysResetReason = 0;

/*系统是否已开始运行*/
static BOOL isOsStarted = FALSE;
/*是否运行串口发送数据*/
static BOOL bComTxEnable = FALSE;

/*基本运行信息*/
baseInfoTypedef tBaseInfo;

static unsigned int taskIdRegistryTable = 0;
static unsigned int taskCount = 0;
static TaskHandle_t comTxTskHandle = NULL;
/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
static void StartBasicTask(void * argument);
static void StartComRxTask(void * argument);
static void StartComTxTask(void * argument);
void OsStartRunning(void);
void PutComToQueue(unsigned char *, unsigned short, unsigned short);
void FlyTaskDeinit(void);
uint8 GetSystemResetReason(void);
void TaskFeedDog(int taskId);
int TaskIdRegistry(void);
void FlyUartGotOne(uint8 data);
/* Private function prototypes -----------------------------------------------*/

/* Extern function prototypes -----------------------------------------------*/

/* Extern function prototypes -----------------------------------------------*/

/* Private function wrapper ---------------------------------------------------------*/
uint32_t FlashLockWrapper(void)
{
	return (uint32_t)HAL_FLASH_Lock();
}
uint32_t FlashUnLockWrapper(void)
{
	return (uint32_t)HAL_FLASH_Unlock();
}
uint32_t FlashProgramWordWrapper(uint32_t address, uint32_t data)
{
	return (uint32_t)HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD ,address ,(uint64_t)data);
}

/* Private function wrapper ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */
/**ca
  * @brief  Create basic task.
  * @param	Null.
  * @retval Null.
  */
__STATIC_INLINE void BasicTaskInit(void)
{
	uint8 xParameters = 0;

	if(xTaskCreate(
					StartBasicTask,
					Basic_TASK_NAME,
					512,
					(void*)&xParameters,
					tskIDLE_PRIORITY + 2,
					&tskHandleBasic
					)!= pdPASS)
	{
		/*Task create failed*/
		#if DEBUG_DG_SEVERE
			FlyDebugPrint(DEBUG_ERROR,"\r\nCreate BasicTask failed");
		#endif
	}
}
/**
  * @brief  Basic task
  * @param	argument .
  * @retval Null.
  */
static void StartBasicTask(void * argument)
{
	static uint32_t dogTimer;
	static uint32_t ledTimer;
	static uint32 busIdleTimer;

	int id = TaskIdRegistry();
	FlyResetUserTimer(&busIdleTimer);

	for(;;)
	{

		/*车辆基本任务，包含车辆方控处理，延时不能太长*/
		vTaskDelay(pdMS_TO_TICKS(10));

		if(FlyReadUserTimer(&ledTimer) >= T_1S)
		{
			LedBlink();
			FlyResetUserTimer(&ledTimer);
		}

		if(FlyReadUserTimer(&dogTimer) >= T_100MS * 10)
		{
			TaskFeedDog(id);
			FlyResetUserTimer(&dogTimer);
		}
	}
}

/**
  * @brief  Create Com rx tasks.
  * @param	Null.
  * @retval Null.
  */
__STATIC_INLINE void ComRxTaskInit(void)
{
	uint8 xParameters = 0;

	xQueueRxFromHost = xQueueCreate(
									configQUEUE_LENGTH_RX_FROM_Host,
									configQUEUE_SIZE_ITEM_RX_FROM_Host);

	if(xTaskCreate(
					StartComRxTask,
					ComRx_TASK_NAME,
					256,
					(void*)&xParameters,
					tskIDLE_PRIORITY + 2,
					&tskHandleComRx
					)!= pdPASS)
	{
		/*Task create failed*/
		#if DEBUG_DG_SEVERE
			FlyDebugPrint(DEBUG_ERROR,"\r\nCreate ComRxTask failed");
		#endif
	}
}
/**
  * @brief  Com rx task
  * @param	argument .
  * @retval Null.
  */
static void StartComRxTask(void * argument)
{
	BaseType_t xResult;
	unsigned char dataReceived;
	TickType_t xMaxBlockTime = pdMS_TO_TICKS(2000);
	uint8 ack;

	uint32_t dogTimer;
	int id = TaskIdRegistry();

	for(;;)
	{
		xResult = xQueueReceive(xQueueRxFromHost, &dataReceived, xMaxBlockTime);

		if(xResult == pdPASS)
		{
			ack = tBaseInfo.HostRxDecode(&dataReceived, 1, &tBaseInfo);

			LedBlink();

			if(ack != 0)
			{
				FlyUartTransmitOne(PORT_TO_HOST, ack);
			}
		}

		/*feed dog*/
		if(FlyReadUserTimer(&dogTimer) >= T_100MS * 10)
		{
			TaskFeedDog(id);
			FlyResetUserTimer(&dogTimer);
		}
	}
}
/**
  * @brief  Create Com tx tasks.
  * @param	Null.
  * @retval Null.
  */
__STATIC_INLINE void ComTxTaskInit(void)
{
	uint8 xParameters = 0;

	xQueueTxToHost = xQueueCreate(
									configQUEUE_LENGTH_TX_TO_Host,
									configQUEUE_SIZE_ITEM_TX_TO_Host);

	if(xTaskCreate(
					StartComTxTask,
					ComTx_TASK_NAME,
					256,
					(void*)&xParameters,
					tskIDLE_PRIORITY + 2,
					&tskHandleComTx
					)!= pdPASS)
	{
		/*Task create failed*/
		#if DEBUG_DG_SEVERE
				FlyDebugPrint(DEBUG_ERROR,"\r\nCreate ComTxTask failed");
		#endif
	}

}
/**
  * @brief  Com Tx Task
  * @param	argument .
  * @retval Null.
  */

static void StartComTxTask(void * argument)
{
	BaseType_t xResult;
	TickType_t xMaxBlockTime = pdMS_TO_TICKS(2000);

	TxToHostFrameTypedef *dataReceived = NULL;

	uint32_t dogTimer;
	int id = TaskIdRegistry();

	for(;;)
	{
		xResult = xQueueReceive(xQueueTxToHost, &dataReceived, xMaxBlockTime);
		comTxTskHandle = xTaskGetCurrentTaskHandle();

		if(xResult == pdPASS)
		{
			if(bComTxEnable)
			{
				LedBlink();

				if(dataReceived != NULL)
				{
					if(dataReceived->frameType == FRAME_TYPE_DATA)
					{
						uint8 tryCount = ACK_REPEAT_CT;

						while(tryCount--)
						{
							/*发送数据帧*/
							FlyUartTransmit(PORT_TO_HOST, (uint8 *)dataReceived->data, dataReceived->length);

							#if HOST_NEED_ACK
							{
								BaseType_t res = pdFALSE;
								uint32_t ulNotifiedValue; 
								
								res =  xTaskNotifyWait(
												0x00,
												0xffffffff, 
												&ulNotifiedValue, 
												pdMS_TO_TICKS(ACK_WAIT_MS));

								/*收到应答，停止重复发送*/
								if((res == pdTRUE) && (ulNotifiedValue & 0x01))
								{
									break;
								}
							}
							#else
								break;
							#endif
						}
					}
				}
			}
		}

		if(xResult == pdPASS)
		{
			/*Free resources*/					
			if(dataReceived != NULL)
			{
				tBaseInfo.Free(dataReceived);
			}
		}
		
		if(FlyReadUserTimer(&dogTimer) >= T_100MS * 10)
		{
			TaskFeedDog(id);
			FlyResetUserTimer(&dogTimer);
		}
	}
}

/*
任务喂狗
par:taskId,分配的任务ID
*/
void TaskFeedDog(int taskId)
{
	static unsigned int tsk = 0;

	tsk |= taskId;

	/*所有任务动成功报道后再喂狗*/
	if(tsk == taskIdRegistryTable)
	{
		FeedDog();
		tsk = 0;
	}
}
/*
注册喂狗任务
return：返回该任务注册的ID，该ID在喂狗时使用
*/
int TaskIdRegistry(void)
{
	int tskId;

	tskId = 0x01 << taskCount;
	taskCount ++;

	/*被注册进入taskIdRegistryTable中的任务必须按时喂狗*/
	taskIdRegistryTable |= tskId;

	/*返回任务id*/
	return tskId;
}
/**
  * @brief  Enable or disable com transmit.
  * @param	state goal state.
  * @retval Null.
  */
void EnableComTx(int state)
{
	FlyDebugPrint(DEBUG_MSG01, "Communication %s", state ? "established" : "canceled");
	bComTxEnable = state ? TRUE : FALSE;
}
/**
  * @brief  任务休眠time ms.
  * @param	Null.
  * @retval Null.
  */
void TaskSleep(int time)
{
	vTaskDelay(pdMS_TO_TICKS(time));
}
/**
  * @brief  获取到应答.
  * @param	Null.
  * @retval Null.
  */
void GotAckFromHost(int ack)
{
#if HOST_NEED_ACK
	FlyDebugPrint(DEBUG_MSG04, "Got %s from host", ack == 0xff ? "ack" : "nack");

	if(ack == 0xff)
	{
		if(comTxTskHandle != NULL)
		{
			/*Notify that ack received from host*/
			xTaskNotify(
					comTxTskHandle,
					0x01, 
					eSetBits);
		}
	}
#endif
}
/**
  * @brief  Parameter and interface init.
  * @param	Null.
  * @retval Null.
  */
void ParameterInit(void)
{
	/*初始化接口*/
	tBaseInfo.McuInitProc = McuInit;
	tBaseInfo.McuDeinitProc = McuDeInit;
	tBaseInfo.McuBasicTask = McuBasicTaskProc;
	tBaseInfo.RxDecodeCallBackDef = HostCmdProc;
	tBaseInfo.HostRxDecode = HostRxDecode;
	tBaseInfo.GotAckFromHost = GotAckFromHost;
	tBaseInfo.ConnectEstablish = EnableComTx;
	tBaseInfo.Sleep = TaskSleep;
	tBaseInfo.Malloc = pvPortMalloc;
	tBaseInfo.Free = vPortFree;

	tBaseInfo.SystemControl.Hibernate = FlySystemEnterSleep;
	tBaseInfo.SystemControl.GetSysResetWhy = GetSystemResetReason;

	tBaseInfo.lowInterface.PeripDeinit = UserPeripheralsDeinit;
	tBaseInfo.lowInterface.transmitToHost = PutComToQueue;
	tBaseInfo.Version.verSize = FLyGetSofVersionStr(tBaseInfo.Version.ver);

	/*Disconect*/
	bComTxEnable = FALSE;

}
/**
  * @brief  Free resources.
  * @param	Null.
  * @retval Null.
  */
void FreeResources(void)
{
	BaseType_t xResult;
	TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);

	/*Free tx queue resources*/
	TxToHostFrameTypedef *dataReceived = NULL;
	
	for(;;)
	{
		xResult = xQueueReceive(xQueueTxToHost, &dataReceived, xMaxBlockTime);

		if(xResult == pdPASS)
		{
			/*Free resources*/					
			if(dataReceived != NULL)
			{
				tBaseInfo.Free(dataReceived);
			}
		}
		else
		{
			break;
		}
	}
}
/**
  * @brief  Deinit work.
  * @param	Null.
  * @retval Null.
  */
void FlyTaskDeinit(void)
{
	/*Disconnect first*/
	bComTxEnable = FALSE;
	isOsStarted = FALSE;

	/*Delete other task*/
	if(xTaskGetHandle(ComRx_TASK_NAME))
	{
		vTaskDelete(xTaskGetHandle(ComRx_TASK_NAME));
	}
	if(xTaskGetHandle(ComRx_TASK_NAME))
	{
		vTaskDelete(xTaskGetHandle(ComTx_TASK_NAME));
	}
	if(xTaskGetHandle("defaultTask"))
	{
		vTaskDelete(xTaskGetHandle("defaultTask"));
	}

	FreeResources();
}

/**
  * @brief  Get system reset reason.
  * @param	Null.
  * @retval Null.
  */
uint8 GetSystemResetReason(void)
{
	return sysResetReason;
}
/**
  * @brief  Check why system reset.
  * @param	Null.
  * @retval Null.
  */
void SystemResetErrCheck(void)
{
	sysResetReason = 0;

	if(RCC->CSR & RCC_CSR_LPWRRSTF)
	{
		sysResetReason |= SYS_RESET_WHY_LPRS;
	}
	if(RCC->CSR & RCC_CSR_WWDGRSTF)
	{
		sysResetReason |= SYS_RESET_WHY_WWDG;
	}
	if(RCC->CSR & RCC_CSR_IWDGRSTF)
	{
		sysResetReason |= SYS_RESET_WHY_FWDG;
	}
	if(RCC->CSR & RCC_CSR_SFTRSTF)
	{
		sysResetReason |= SYS_RESET_WHY_SWRS;
	}
	if(RCC->CSR & RCC_CSR_PORRSTF)
	{
		sysResetReason |= SYS_RESET_WHY_PORR;
	}
	if(RCC->CSR & RCC_CSR_PINRSTF)
	{
		sysResetReason |= SYS_RESET_WHY_EPRS;
	}

	RCC->CSR |= RCC_CSR_RMVF;

	if(sysResetReason)
	{
		FlyDebugPrint(DEBUG_ERROR, "Reset Error %x", sysResetReason);
	}
}
/**
  * @brief  初始化任务与参数.
  * @param	Null.
  * @retval Null.
  */
void FlyTaskInit(void)
{
	SystemResetErrCheck();

	FlyDebugPrint(DEBUG_MSG01, "========Hello Flysystem========");

	/*初始化接口和参数*/
	ParameterInit();

	/*创建任务*/
	BasicTaskInit();
	ComRxTaskInit();
	ComTxTaskInit();

}


/**
  * @brief  串口中断回调函数.
  * @param 	port 数据来源的端口.
  * @param	data 数据本身.
  * @retval Null.
  */
void FlyUartGotOne(uint8 data)
{

	if(isOsStarted)
	{
		/*在中断上下文中，不能延时*/
		xQueueSendToBackFromISR(xQueueRxFromHost, &data, NULL);
	}

}

/**
  * @brief  系统开始运行.
  * @param  Null.
  * @retval Null.
  */
void OsStartRunning(void)
{
	isOsStarted = TRUE;

	/*Enable irq*/
	UserIrqStart();
}

/**
  * @brief  将发送给host的数据放进发送队列.
  * @param  pData 数据的引用地址.
  * @param  length 数据长度.
  * @param  wait 阻塞时间.
  * @retval Null.
  */
void PutComToQueue(unsigned char * const pData, unsigned short length, unsigned short wait)
{
	TxToHostFrameTypedef *pFrame;
	
	pFrame = (TxToHostFrameTypedef *)tBaseInfo.Malloc(2 + length);
	
	if(pFrame == NULL)
	{
		return;
	}

	pFrame->frameType = FRAME_TYPE_DATA;
	pFrame->length = length;
	memcpy(pFrame->data, pData, length);

	/*Put into tx queue*/
	if(isOsStarted)
	{
		uint8 res = xQueueSend(xQueueTxToHost, &pFrame, wait);
		if( res != pdTRUE)
		{
			tBaseInfo.Free(pFrame);
		}
	}
	else
	{
		tBaseInfo.Free(pFrame);
	}
}
/* USER CODE END 0 */

