/*****************************************************************************
1.(主 Task)
2.(从 )
******************************************************************************/
#include "slaveTool.h"

/* Chip parameters ------------------------------------------------------------*/
#define 	FLASH_BASE_ADDR							(0x08000000)
#define 	FLASH_USER_START_ADDR					(0x08002800)
#define 	SYSTEM_BASE_ADDR						(FLASH_USER_START_ADDR + 0x100)
#define 	SRAM_SIZE								(0x5000)		/*芯片SRAM大小，STM32F103C8 Sram = 20k*/
#define		FLASH_SIZE								(0x10000)		/*芯片flash大小，STM32F103C8  64k*/
#define 	SRAM_BASE_ADDR							(0x20000000)
#define 	PARAMETER_START_ADDR					(SRAM_BASE_ADDR + SRAM_SIZE - 0x100)
/* ---------------------------------------------------------------------------- */

typedef struct
{
	unsigned int RebootState;
	unsigned int BootCmd;
	unsigned int Vol[4];
}structSysData;

/*系统保留的参数分区，不被编译器分配*/
#define 	tpDataInfo	((structSysData *)(PARAMETER_START_ADDR))	//256 bytes to store system parameters

static BOOL rebootFlag = FALSE;
static uint32_t rebootTime;

static BOOL heartFlag = FALSE;
static uint32_t PingSendTimer;

extern IWDG_HandleTypeDef hiwdg;

extern BaseType_t UartTransmitDataToHost(uint8_t * Buf, uint16_t Len);

/*****************************************************************************
**Name:		 	ReadUserTimer
**Function:
**Args:
**Return:
******************************************************************************/
void ResetUserTimer(uint32_t *Timer)
{
	*Timer = HAL_GetTick();
}
/*****************************************************************************
**Name:		 	ReadUserTimer
**Function:
**Args:
**Return:
******************************************************************************/
uint32_t ReadUserTimer(uint32_t *Timer)
{
	uint32_t tmp = HAL_GetTick();
	return (*Timer <= tmp) ? (tmp - *Timer) : (0xFFFFFFFF - tmp + *Timer);
}
/*****************************************************************************
**Name:		 	Delay1ms
**Function:
**Args:
**Return:
******************************************************************************/
void Delay1ms(volatile uint32_t nTime)
{
	HAL_Delay(nTime);
}

/*****************************************************************************
**Name:		 	MX_GPIO_Write
**Function:
**Args:
**Return:
******************************************************************************/
void MX_GPIO_Write(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, PinState);
}
void MX_GPIO_Toggle(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_TogglePin(GPIOx, GPIO_Pin);
}

/*****************************************************************************
**Name:		 	MX_IWDG_Refresh
**Function:
**Args:
**Return:
******************************************************************************/
void MX_IWDG_Refresh(void)
{
  HAL_IWDG_Refresh(&hiwdg);
}
/*****************************************************************************
**Name:		 	SendHeartBeatToHost
**Function: 发送心跳函数
**Args:
**Return:
******************************************************************************/
void SendHeartBeatToHost(void)
{
	/*心跳*/
	uint8 sendBuf[] = {0x01, 0x01, 0xFF};
	static uint32_t timer;

	if(ReadUserTimer(&timer) >= T_100MS * 10)
	{
		UartTransmitDataToHost(sendBuf, sizeof(sendBuf));

		ResetUserTimer(&timer);
	}
}

/*****************************************************************************
**Name:		 	HeartBeatCheck
**Function:	心跳应答函数
**Args:
**Return:
******************************************************************************/
void HeartBeatCheck(unsigned char * const pCAN_RxData)
{
	if ((pCAN_RxData[1] == 0x01) && (pCAN_RxData[2] == 0x01) )
	{
		/* code */
		heartFlag = TRUE;
//		uint8 sendBuf[] = {0xAC, 0x10, 0x01, 0x01};
//		UartTransmitDataToHost(sendBuf, sizeof(sendBuf));
		ResetUserTimer(&PingSendTimer);
	}
	else
	{
		/* code */
	}
}

/*****************************************************************************
**Name:		 	IsHeartBroken
**Function:	检测心跳是否丢失，如果心跳超时，则重新请求
**Args:
**Return:
******************************************************************************/
void IsHeartBroken(void)
{
	if(heartFlag == FALSE)
	{
		ResetUserTimer(&PingSendTimer);
	}
	else
	{
		if((heartFlag == TRUE) && (ReadUserTimer(&PingSendTimer) >= T_1S * 15))
		{
			heartFlag = FALSE;
		}
	}
}

/*****************************************************************************
**Name:		 	RebootDevice
**Function:	此函数需放在base任务中运行
**Args:
**Return:
******************************************************************************/
void RebootDevice()
{
	static uint32_t rebootSystemTimer;

	if(rebootFlag == FALSE)
	{
		ResetUserTimer(&rebootSystemTimer);
	}
	else
	{
		if(ReadUserTimer(&rebootSystemTimer) >= rebootTime)
		{
			HAL_NVIC_SystemReset();
			ResetUserTimer(&rebootSystemTimer);
		}
	}
}

/**
  * @brief  解析host协议.
  * @param	Buf 一帧数据中数据部分的首地址.
  * @param	Len 该数据帧包含的数据长度.
  * @retval Null.
  */
void HostCmdProcess(uint8_t *Buf, uint16_t Len)
{
    uint8_t *Cmd;
    Cmd = Buf;
    
    dbgprintf("Process CMD %#02X\n", Cmd[0]);
    
    switch(Cmd[0])
    {
        case 0x00:
        break;
        
        case 0x01:
        {
            if(Cmd[1] == 0xFF)
            {
                /*System reset*/
                uint8_t ack[] = {0x01, 0xFF, 0x00};
				
                dbgprintf("System reset...\n");
                UartTransmitDataToHost(ack, sizeof(ack));

				rebootFlag = TRUE;
				rebootTime = Cmd[2] <<8 | Cmd[3];
            }
			
			if((Cmd[1] == 0x01) && (Cmd[2] == 0x01))
            {
                /*Got ping*/
                uint8_t ack[] = {0x01, 0x01, 0x00};
                dbgprintf("Got ping...\n");
                UartTransmitDataToHost(ack, sizeof(ack));
            }
        }
        break;
        
        case 0x0A:
        {
		  	//
		  	if(Cmd[1] == 0x01)
			{
			}
			
		  	//
		  	if(Cmd[1] == 0x0D)
			{
			}
			
		  	//
		  	if(Cmd[1] == 0xAD)
			{
			}			
        }
        break;
        
        case 0x0B:
        {
		  	//
		  	if(Cmd[1] == 0x02)
			{
			}
			
		  	//
		  	if(Cmd[1] == 0x03)
			{
			}
			
		  	//
		  	if(Cmd[1] == 0x04)
			{
			}
		}
        break;
		
        case 0xAC:
        {

        }
        break;
		
		default:
			break;
    }
}
	
/**
  * @brief  基本任务.
  * @retval Null.
  */
void McuBasicTaskProc(void)
{
	/*重启设备*/
	RebootDevice();

	/*检测心跳是否丢失*/
	IsHeartBroken();
	
	/*发送心跳函数*/
	SendHeartBeatToHost();
}

/**
  * @brief  配置逆初始化,系统调用.
  * @retval Null.
  */
void McuDeInit(void)
{

}

/**
  * @brief  初始化,系统调用.
  * @retval Null.
  */
void McuInit(void)
{

}

