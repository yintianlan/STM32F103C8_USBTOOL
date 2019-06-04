#include <stdio.h>
#include <string.h>

#include "interface.h"
#include "tinyprintf.h"

#include "def.h"
#include "FlyConfig.h"
#include "usbd_cdc_if.h"

/* 存储地址参数 */
#define 	USER_WANT2UPDATE_PARA				(0x5A5A5A5A)
#define 	SYSTEM_HARDFAULT_REBOOT				(0x1A1A1A1A)
#define		SYSTEM_UPADTE_DONE_BOOT				(0X2A2A2A2A)


/* Chip parameters ------------------------------------------------------------*/
#define 	FLASH_BASE_ADDR							(0x08000000)
#define 	FLASH_USER_START_ADDR					(0x08002800)
#define 	SYSTEM_BASE_ADDR						(FLASH_USER_START_ADDR + 0x100)
#define 	SRAM_SIZE								(0x5000)		/*芯片SRAM大小，STM32F103C8 Sram = 20k*/
#define		FLASH_SIZE								(0x10000)		/*芯片flash大小，STM32F103C8  64k*/
#define 	SRAM_BASE_ADDR							(0x20000000)
#define 	PARAMETER_START_ADDR					(SRAM_BASE_ADDR + SRAM_SIZE - 0x100)
/* ---------------------------------------------------------------------------- */
#define		SYSTEM_BOOT_CMD_PWR_PWERON				(0x1A1A1A1A)		//上电启动
#define		SYSTEM_BOOT_CMD_UPDATE_PWRON			(0X2A2A2A2A)		//升级完成启动
#define		SYSTEM_BOOT_CMD_WAKEUP_PWRON			(0X1bf53ae9)		//休眠唤醒启动
/* ---------------------------------------------------------------------------- */
#define 	SYSTEM_SHUTDOWN_SLEEP					(SYSTEM_BOOT_CMD_WAKEUP_PWRON)
#define		SYSTEM_SHUTDOWN_HALT					(SYSTEM_BOOT_CMD_PWR_PWERON)
/* ---------------------------------------------------------------------------- */
#define		FLYAUDIO_STR						"FlyAudio"

typedef struct
{
	unsigned int UserWant2Update;
	unsigned int RebootState;
	unsigned int BootCmd;
	unsigned int BootArg;
	unsigned int iwdgMark;
}structSysParameter;

enumBootState bootState = BOOT_PWR_ON;						//启动命令

/*系统保留的参数分区，不被编译器分配*/
#define 	tpParaInfo	((structSysParameter *)(PARAMETER_START_ADDR))	//256 bytes to store system parameters

/*跳转函数指针*/
typedef void (*pFunction)(void);

/* 编译时间信息 */
#define DIGIT(s, no) ((s)[no] - '0')

int hours = (10 * DIGIT(__TIME__, 0) + DIGIT(__TIME__, 1));
int minutes = (10 * DIGIT(__TIME__, 3) + DIGIT(__TIME__, 4));
int seconds = (10 * DIGIT(__TIME__, 6) + DIGIT(__TIME__, 7));
/* WARNING: This will fail in year 10000 and beyond, as it assumes
* that a year has four digits. */
int year = ( 1000 * DIGIT(__DATE__, 7)
+ 100 * DIGIT(__DATE__, 8)
+ 10 * DIGIT(__DATE__, 9)
+ DIGIT(__DATE__, 10));

/*
* Jan - 1
* Feb - 2
* Mar - 3
* Apr - 4
* May - 5
* Jun - 6
* Jul - 7
* Aug - 8
* Sep - 9
* Oct - 10
* Nov - 11
* Dec - 12
*/

/* Use the last letter as primary "key" and middle letter whenever
* two months end in the same letter. */
const int months = (__DATE__[2] == 'b' ? 2 :
(__DATE__[2] == 'y' ? 5 :
(__DATE__[2] == 'l' ? 7 :
(__DATE__[2] == 'g' ? 8 :
(__DATE__[2] == 'p' ? 9 :
(__DATE__[2] == 't' ? 10 :
(__DATE__[2] == 'v' ? 11 :
(__DATE__[2] == 'c' ? 12 :
(__DATE__[2] == 'n' ?
(__DATE__[1] == 'a' ? 1 : 6) :
/* Implicit "r" */
(__DATE__[1] == 'a' ? 3 : 4))))))))));
const int day = ( 10 * (__DATE__[4] == ' ' ? 0 : DIGIT(__DATE__, 4))
+ DIGIT(__DATE__, 5));

BOOL	wakedUpByRtc;

extern ADC_HandleTypeDef hadc1;

extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
extern void FlyUartGotOne(uint8 data);

/*获取数组的长度*/
#define GetLengthOf(data, type)		(sizeof(data)/sizeof(type))

/*****************************************************************************
**Name:		 	ReadUserTimer
**Function:
**Args:
**Return:
******************************************************************************/
void FlyResetUserTimer(uint32_t *Timer)
{
	if(Timer != NULL)
	{
		*Timer = HAL_GetTick();
	}
}
/*****************************************************************************
**Name:		 	ReadUserTimer
**Function:
**Args:
**Return:
******************************************************************************/
uint32_t FlyReadUserTimer(uint32_t *Timer)
{
	if(Timer != NULL)
	{
		uint32_t tmp = HAL_GetTick();
		uint32_t res = (*Timer <= tmp) ? (tmp - *Timer) : (0xFFFFFFFF - tmp + *Timer);
		return res;
	}

    return 0;
}
/*****************************************************************************
**Name:		 	FlyGetCompileDate
**Function:		Get compile date
**Args:
**Return:
******************************************************************************/
void FlyGetCompileDate(char *src, int length)
{
	char monthHigh = months/10 + '0';
	char monthLow  = months%10 + '0';

	char date[] =
	{
		__DATE__[9] == 32 ? '0' : __DATE__[9],
		__DATE__[10] == 32 ? '0' : __DATE__[10],
		monthHigh,
		monthLow,
		__DATE__[4] == 32 ? '0' : __DATE__[4],
		__DATE__[5] == 32 ? '0' : __DATE__[5],
	};

	if(length < 6)
	{
		memcpy(src, date, length);
	}
	else
	{
		memcpy(src, date, 6);
	}
}
/*****************************************************************************
**Name:		 	获取软件版本号的字节表示
**Function:
**Args:			pOut :输出数组首地址
**Return:		返回输出的字节数
******************************************************************************/
int FLyGetSofVersionStr(unsigned char * const pOut)
{
	uint8 position;

	memcpy(pOut, FLYAUDIO_STR, sizeof(FLYAUDIO_STR));
	position = sizeof(FLYAUDIO_STR);
	pOut[position - 1] = '#';

	memcpy(pOut + position, VerCarTypeStr, sizeof(VerCarTypeStr));
	position += sizeof(VerCarTypeStr);
	pOut[position - 1] = '#';

	memcpy(pOut + position, VerSoftwareStr, sizeof(VerSoftwareStr));

	position += sizeof(VerSoftwareStr) - 1;

	return position;
}
/*****************************************************************************
**Name:		 	FlySystemJump2Where
**Function:		Jump to specific location
**Args:
**Return:
******************************************************************************/
int FlySystemJump2Where(uint32_t address)
{
	/* Test if user code is programmed starting from address "APPLICATION_ADDRESS" */
	if (((*(__IO uint32_t*)address) & 0x2FFE0000 ) == 0x20000000)
	{
		extern void vJumpToWhere(int);
		vJumpToWhere(address);
	}

	/*跳转失败，返回-1*/
	return -1;
}
/*****************************************************************************
**Name:		 	FlyGoToUpdate
**Function:	 	Enter bootloader for updating
**Args:
**Return:
******************************************************************************/
void FlyGoToUpdate(uint8 arg)
{
	/*Boot parameter that for update*/
	tpParaInfo->UserWant2Update = USER_WANT2UPDATE_PARA;
	tpParaInfo->BootArg = arg;
	
	FlySystemJump2Where(FLASH_BASE_ADDR);
}

/*****************************************************************************
**Name:		 	HanldeReceiveData
**Function:	USB虚拟串口接收数据
**Args:
**Return:
******************************************************************************/
void HanldeReceiveData(uint8_t* Buf, uint32_t Len)
{
	unsigned char * pdata = Buf;
 	unsigned int  length = Len;
	static uint8 data;
  
	if(length != 0)
	{
		for(; length > 0; length--)
		{
			data = *pdata++;
			FlyUartGotOne(data);
		}
	}
}


/*****************************************************************************
**Name:		 	FlyUartTransmitOne
**Function:
**Args:
**Return:
******************************************************************************/
#if 0
void FlyUartTransmitOne(enumUartPort port, uint8 data)
{
	USART_TypeDef *pDev;

	switch(port)
	{
		case PORT_TO_HOST:
				pDev = USART_HOST;
			break;

		case PORT_TO_EXT:
				pDev = USART_EXT;
			break;

		case PORT_TO_DEBUG:
				pDev = USART_DEBUG;
			break;

		default:
			return;
	}

	LL_USART_TransmitData8(pDev, data);
	while(!LL_USART_IsActiveFlag_TC(pDev));
}
#else
void FlyUartTransmitOne(enumUartPort port, uint8 data)
{
	CDC_Transmit_FS(&data, 1);
}
#endif
/*****************************************************************************
**Name:		 	FlyUartTransmitDatas
**Function:	 	Transmit amount of data in blocking mode
**Args:
**Return:
******************************************************************************/
void FlyUartTransmit(enumUartPort port, uint8 *pdata, uint16 size)
{
	u16 currentCount = 0;
	uint8 *data = pdata;

	if(size == 0)
	{
		return;
	}

	if(data != NULL)
	{
		TaskSuspendSchedule;

		while(1)
		{
			FlyUartTransmitOne(port, *data);

			currentCount++;
			if(currentCount < size)
			{
				data++;
			}
			else
			{
				break;
			}
		}

		TaskResumeSchedule;
	}
}
/*****************************************************************************
**Name:		 	FlyUartTransmitDatas
**Function:	 	Transmit amount of data in blocking mode
**Args:
**Return:
******************************************************************************/
void FlyUartTransmitToHost(uint8 *pData, uint16 size)
{
	FlyUartTransmit(PORT_TO_HOST, pData, size);
}

/*****************************************************************************
**Name:		 	FlyUartGotOne
**Function:
**Args:
**Return:
******************************************************************************/
//__weak void FlyUartGotOne(enumUartPort port, uint8 data)
//{
//
//}
/*****************************************************************************
**Name:		 	FLySystemErrReset
**Function:
**Args:
**Return:
******************************************************************************/
void FLySystemErrReset(void)
{
	#if DEBUG_DG_SEVERE
	printf("FlySystem error, rebooting...");
	#endif

	tpParaInfo->RebootState = SYSTEM_HARDFAULT_REBOOT;

	HAL_NVIC_SystemReset();
}
/*****************************************************************************
**Name:		 	fputc
**Function:
**Args:
**Return:
******************************************************************************/
static void stdout_putf(void *unused, char c)
{
#if PRINTF_ENABLE
  UNUSED(unused);
  FlyUartTransmitOne(PORT_TO_DEBUG, (uint8)c);
#endif
}
/*****************************************************************************
**Name:		 	InterfaceInit
**Function:
**Args:
**Return:
******************************************************************************/
void InterfaceInit(void)
{
	uint32 bootCmd = tpParaInfo->BootCmd;
	tpParaInfo->BootCmd = 0;
	
#if PRINTF_ENABLE
	init_printf(NULL, stdout_putf);
#endif

	/* Clear all pending interrupt */
	if(READ_BIT(EXTI->PR, 0xffffffff))
	{
		SET_BIT(EXTI->PR, 0xffffffff);
	}

	/*Clear update mark*/
	tpParaInfo->UserWant2Update = 0;

	/*Get boot state*/
	if(bootCmd == SYSTEM_BOOT_CMD_PWR_PWERON)
	{
		bootState = BOOT_PWR_ON;
	}
	else if(bootCmd == SYSTEM_BOOT_CMD_UPDATE_PWRON)
	{
		bootState = BOOT_UPDATE_ON;
	}
	else if(bootCmd == SYSTEM_BOOT_CMD_WAKEUP_PWRON)
	{
		bootState = BOOT_WAKEUP_ON;
	}
	else
	{
		bootState = BOOT_PWR_ON;
	}
}
/*****************************************************************************
**Name:		 	获取启动状态
**Function:
**Args:
**Return:
******************************************************************************/
enumBootState GetBootState(void)
{
	return bootState;
}

/*****************************************************************************
**Name:		 	FlySystemEnterSleep
**Function:		进入休眠
**Args:	wkSrc	唤醒源
**Args:	method	唤醒后操作
**Args:	pBaseInfo	全局基本信息
**Return:
******************************************************************************/
void FlySystemEnterSleep(int wkSrc, int method, void * const pBaseInfo)
{	
	
	int res = 0;

	FlyDebugPrint(DEBUG_MSG01, "System enter sleep %02x %02x", wkSrc, method);

	/* After wakeup and go to where */
#if HAVE_BOOTLOADER
	res = FlySystemJump2Where(SYSTEM_BASE_ADDR);
#else
	res = FlySystemJump2Where(FLASH_BASE_ADDR);
#endif

	if(res < 0)
	{
		/*Jump error*/
		FLySystemErrReset();
	}
}

/*
外设Deinit
*/
void UserPeripheralsDeinit(void)
{
	vTaskSuspendAll();

	FlyDebugPrint(DEBUG_MSG01, "Deinit Peripherals...");

	HAL_ADC_DeInit(&hadc1);

	/*Turn off led*/
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
}

